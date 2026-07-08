#include "bot_mode.hpp"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "VkSmol/platform/headless_platform.hpp"
#include "dpp/dpp.h"

#include "app/log.hpp"
#include "application.hpp"
#include "offline/job_queue.hpp"
#include "utils/progress.hpp"

struct PendingRender {
    JobQueue       queue;
    dpp::snowflake channel_id;
};

static std::string logPrefix(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "[DBG]";
        case LogLevel::Info:    return "[INF]";
        case LogLevel::Success: return "[SUC]";
        case LogLevel::Warn:    return "[WRN]";
        case LogLevel::Error:   return "[ERR]";
    }
    return "     ";
}

static std::string readFileBinary(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

static std::string stripTerminalCodes(std::string s) {
    if (!s.empty() && s.front() == '\r') s.erase(0, 1);
    if (auto p = s.rfind("\033[K"); p != std::string::npos) s.erase(p);
    return s;
}

void runBotMode(const std::string& token) {
    std::queue<PendingRender> work_queue;
    std::mutex                work_mtx;
    std::condition_variable   work_cv;

    std::vector<std::string> log_buffer;
    std::mutex               log_mtx;

    Log::setConsumer([&](const LogEntry& e) {
        std::lock_guard lock(log_mtx);
        log_buffer.push_back(logPrefix(e.level) + " [" + e.source + "] " + e.message);
    });

    HeadlessPlatform platform(1280, 720);
    Application      app(platform);

    dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);

    bot.on_message_create([&](const dpp::message_create_t& event) {
        if (!event.msg.content.starts_with("!render")) return;

        const dpp::snowflake channel_id = event.msg.channel_id;

        if (event.msg.attachments.empty()) {
            bot.message_create(dpp::message{channel_id, "Attach a job `.json` file."});
            return;
        }

        const auto& att = event.msg.attachments[0];
        if (!att.filename.ends_with(".json")) {
            bot.message_create(dpp::message{channel_id, "Attachment must be a `.json` file."});
            return;
        }

        bot.request(att.url, dpp::m_get, [&, channel_id](const dpp::http_request_completion_t& res) {
            if (res.status != 200) {
                bot.message_create(dpp::message{channel_id, "Failed to download attachment."});
                return;
            }

            try {
                auto tmp = std::filesystem::temp_directory_path() / "vkray_bot_job.json";
                { std::ofstream f(tmp); f << res.body; }
                JobQueue queue = JobQueue::fromFile(tmp);
                std::filesystem::remove(tmp);

                for (const auto& job : queue.entries())
                    std::filesystem::create_directories(job.output.parent_path());

                int position;
                {
                    std::lock_guard lock(work_mtx);
                    position = static_cast<int>(work_queue.size()) + 1;
                    work_queue.push({std::move(queue), channel_id});
                }
                work_cv.notify_one();

                std::string reply = (position == 1)
                    ? "Render started."
                    : std::format("Queued (position {}).", position);
                bot.message_create(dpp::message{channel_id, reply});

            } catch (const std::exception& e) {
                bot.message_create(dpp::message{channel_id, std::format("Invalid job: {}", e.what())});
            }
        });
    });

    bot.start(dpp::st_return);
    Log::info("Bot", "Discord bot started, waiting for render jobs.");

    while (true) {
        PendingRender item = [&] {
            std::unique_lock lock(work_mtx);
            work_cv.wait(lock, [&] { return !work_queue.empty(); });
            auto front = std::move(work_queue.front());
            work_queue.pop();
            return front;
        }();

        std::promise<dpp::message> status_promise;
        bot.message_create(dpp::message{item.channel_id, "```\nStarting…\n```"},
            [&status_promise](const dpp::confirmation_callback_t& cb) {
                if (cb.is_error())
                    status_promise.set_value({});
                else
                    status_promise.set_value(std::get<dpp::message>(cb.value));
            });
        dpp::message status = status_promise.get_future().get();

        {
            std::lock_guard lock(log_mtx);
            log_buffer.clear();
        }

        auto last_update = std::chrono::steady_clock::now();

        auto flush = [&](const std::string& progress_line = "") {
            std::string body;
            {
                std::lock_guard lock(log_mtx);
                size_t start = log_buffer.size() > 15 ? log_buffer.size() - 15 : 0;
                for (size_t i = start; i < log_buffer.size(); ++i)
                    body += log_buffer[i] + "\n";
            }
            if (!progress_line.empty()) body += "\n" + progress_line;
            if (body.empty()) body = "…";
            if (body.size() > 1900) body = "…" + body.substr(body.size() - 1900);
            status.content = "```\n" + body + "\n```";
            bot.message_edit(status, [](const dpp::confirmation_callback_t&) {});
            last_update = std::chrono::steady_clock::now();
        };

        const uint32_t totalSamples = item.queue.entries().empty() ? 0 : item.queue.entries().front().samples;
        std::ostringstream bar_stream;
        ProgressBar bar("", totalSamples, " spp", 20, bar_stream);

        item.queue.onProgress = [&](float progress, const Job& job) {
            if (std::chrono::steady_clock::now() - last_update < std::chrono::seconds(2)) return;
            bar.update(static_cast<uint32_t>(progress * static_cast<float>(job.samples)));
            flush(stripTerminalCodes(bar_stream.str()));
            bar_stream.str({});
            bar_stream.clear();
        };

        app.runJobs(item.queue);

        flush();

        for (const auto& job : item.queue.entries()) {
            if (!std::filesystem::exists(job.output)) continue;
            dpp::message result{item.channel_id, ""};
            result.add_file(job.output.filename().string(), readFileBinary(job.output));
            bot.message_create(result, [](const dpp::confirmation_callback_t&) {});
        }
    }
}
