#pragma once

#include "core/scene/object/material.hpp"

bool drawPrincipledUI(Material& mat);
bool drawEmissiveUI(Material& mat);
bool drawLambertianUI(Material& mat);
bool drawGgxMetalUI(Material& mat);
bool drawGgxGlossyUI(Material& mat);
bool drawDielectricUI(Material& mat);
bool drawVolumeUI(Material& mat);
bool drawProgrammableUI(Material& mat);

bool drawMaterialUI(Material& mat);