import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = []
CODEOWNERS = ["@bestander"]

arduino_gfx_display_ns = cg.esphome_ns.namespace("arduino_gfx_display")
ArduinoGFXDisplay = arduino_gfx_display_ns.class_("ArduinoGFXDisplay", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ArduinoGFXDisplay),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Add the Arduino_GFX library dependency
    cg.add_library("moononournation/GFX Library for Arduino", "1.6.0")
    
    # Add Wire library for I2C support
    cg.add_library("Wire", None)
