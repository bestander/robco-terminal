
import esphome.codegen as cg
import esphome.config_validation as cv


hello_world_ns = cg.esphome_ns.namespace('hello_world')
HelloWorldComponent = hello_world_ns.class_('HelloWorldComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(HelloWorldComponent),
})

def to_code(config):
    var = cg.new_Pvariable(config["id"])
    yield var
    yield cg.register_component(var, config)

# Help ESPHome recognize the external component
hello_world = HelloWorldComponent
