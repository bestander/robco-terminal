import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_ADDRESS

DEPENDENCIES = []
AUTO_LOAD = []

uart_logger_ns = cg.esphome_ns.namespace('uart_logger')
UARTLogger = uart_logger_ns.class_('UARTLogger', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(UARTLogger),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)