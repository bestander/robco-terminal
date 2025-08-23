
import esphome.codegen as cg
import esphome.config_validation as cv

robco_display_ns = cg.esphome_ns.namespace('robco_display')
RobcoDisplayComponent = robco_display_ns.class_('RobcoDisplayComponent', cg.Component)

# Import pico_io_extension namespace and class
from ..pico_io_extension import pico_io_ns, PicoIOExtension
from esphome.components import switch

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RobcoDisplayComponent),
    cv.Optional("pico_io_extension"): cv.use_id(PicoIOExtension),
    cv.Optional("red_light_pin", default=17): cv.int_,
    cv.Optional("green_light_pin", default=21): cv.int_,
})

def to_code(config):
    var = cg.new_Pvariable(config["id"])
    yield var
    if "pico_io_extension" in config:
        ext = yield cg.get_variable(config["pico_io_extension"])
        cg.add(var.set_pico_io_extension(ext))
    cg.add(var.set_red_light_pin(config.get("red_light_pin", 17)))
    cg.add(var.set_green_light_pin(config.get("green_light_pin", 21)))
    yield cg.register_component(var, config)

robco_display = RobcoDisplayComponent
