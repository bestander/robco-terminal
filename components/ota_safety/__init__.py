import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import web_server_base
from esphome.const import CONF_ID
from esphome.core import CORE, coroutine

CODEOWNERS = ["@bestander"]
DEPENDENCIES = ["web_server_base", "preferences"]

ota_safety_ns = cg.esphome_ns.namespace("ota_safety")
OTASafety = ota_safety_ns.class_("OTASafety", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(OTASafety),
        cv.Optional("enable_http_endpoints", default=True): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine
def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    cg.add(var.set_enable_http_endpoints(config["enable_http_endpoints"]))
