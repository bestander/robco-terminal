import esphome.codegen as cg
import esphome.config_validation as cv

pico_io_ns = cg.esphome_ns.namespace('pico_io_extension')
PicoIOExtension = pico_io_ns.class_('PicoIOExtension', cg.Component)

CONFIG_SCHEMA = cv.Schema({
	cv.GenerateID(): cv.declare_id(PicoIOExtension),
	cv.Optional("rx_pin", default=17): cv.int_,
	cv.Optional("tx_pin", default=18): cv.int_,
})

def to_code(config):
	var = cg.new_Pvariable(config["id"])
	yield var
	yield cg.register_component(var, config)
	cg.add(var.set_uart_pins(config["rx_pin"], config["tx_pin"]))
