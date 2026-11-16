import "utils.slang";
import "scene.slang";

[anyValueSize(4)]
interface ISky {
    float3 evaluate(float3 dir);
}

extension<T : ISky> Script<T> : ISky {
    float3 evaluate(float3 dir) {
        return loadParams<T>(base).evaluate(dir);
    }
}

struct DefaultSky : ISky {
    float3 evaluate(float3 dir) {
        return float3(0.1);
    }
}

float3 skyColor(int skySlot, int skyParamsBase, float3 dir) {
    ISky sky = createDynamicObject<ISky, uint>(uint(skySlot), uint(skyParamsBase));
    return sky.evaluate(dir);
}
