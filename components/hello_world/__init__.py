
import esphome.codegen as cg
import esphome.config_validation as cv

hello_world_ns = cg.esphome_ns.namespace('hello_world')
HelloWorldComponent = hello_world_ns.class_('HelloWorldComponent', cg.Component)

# Import pico_io_extension namespace and class
from ..pico_io_extension import pico_io_ns, PicoIOExtension

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HelloWorldComponent),
    cv.Optional("pico_io_extension"): cv.use_id(PicoIOExtension),
})

def to_code(config):
    var = cg.new_Pvariable(config["id"])
    yield var
    if "pico_io_extension" in config:
        ext = yield cg.get_variable(config["pico_io_extension"])
        cg.add(var.set_pico_io_extension(ext))
    yield cg.register_component(var, config)

# Help ESPHome recognize the external component
hello_world = HelloWorldComponent
