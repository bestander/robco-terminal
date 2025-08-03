import esphome.codegen as cg
import esphome.config_validation as cv
# from esphome.components import display  # Removed since we use integrated Arduino_GFX
# from esphome.components import mqtt  # Temporarily removed for debugging
from esphome.const import CONF_ID, CONF_LAMBDA
from esphome import pins

DEPENDENCIES = []
# DEPENDENCIES = ["display", "mqtt"]  # Temporarily removed for debugging
CODEOWNERS = ["@your_github_username"]

robco_terminal_ns = cg.esphome_ns.namespace("robco_terminal")
RobCoTerminal = robco_terminal_ns.class_("RobCoTerminal", cg.Component)

CONF_KEYBOARD_ID = "keyboard_id"
CONF_USB_DP_PIN = "usb_dp_pin"
CONF_USB_DM_PIN = "usb_dm_pin"
CONF_DOWN_BUTTON_PIN = "down_button_pin"
CONF_ENTER_BUTTON_PIN = "enter_button_pin"
CONF_BACK_BUTTON_PIN = "back_button_pin"
CONF_MQTT_TOPIC_PREFIX = "mqtt_topic_prefix"
CONF_BOOT_SEQUENCE = "boot_sequence"
CONF_CURSOR_BLINK = "cursor_blink"
CONF_FONT_COLOR = "font_color"
CONF_BACKGROUND_COLOR = "background_color"
CONF_MENU_ITEMS = "menu_items"

# Menu item types
CONF_TITLE = "title"
CONF_TYPE = "type"
CONF_ITEMS = "items"
CONF_MQTT_TOPIC = "mqtt_topic"
CONF_MQTT_PAYLOAD = "mqtt_payload"
CONF_READONLY = "readonly"
CONF_CONDITION_TOPIC = "condition_topic"
CONF_CONDITION_VALUE = "condition_value"
CONF_FILE_PATH = "file_path"
CONF_MAX_ENTRIES = "max_entries"

MENU_TYPES = ["submenu", "action", "status", "text_editor"]

def validate_menu_item(config):
    """Validate menu item configuration based on type."""
    item_type = config[CONF_TYPE]
    
    if item_type == "action":
        if CONF_MQTT_TOPIC not in config:
            raise cv.Invalid("Action items must have mqtt_topic")
        if CONF_MQTT_PAYLOAD not in config:
            raise cv.Invalid("Action items must have mqtt_payload")
    elif item_type == "status":
        if CONF_MQTT_TOPIC not in config:
            raise cv.Invalid("Status items must have mqtt_topic")
    elif item_type == "submenu":
        if CONF_ITEMS not in config:
            raise cv.Invalid("Submenu items must have items")
    elif item_type == "text_editor":
        if CONF_FILE_PATH not in config:
            raise cv.Invalid("Text editor items must have file_path")
    
    return config

# Define menu item schema without recursive reference for now
# ESPHome doesn't handle deep recursive schemas well, so we'll limit to 2 levels
MENU_ITEM_SCHEMA = cv.All(
    cv.Schema({
        cv.Required(CONF_TITLE): cv.string,
        cv.Required(CONF_TYPE): cv.one_of(*MENU_TYPES, lower=True),
        cv.Optional(CONF_ITEMS): cv.ensure_list(
            cv.All(
                cv.Schema({
                    cv.Required(CONF_TITLE): cv.string,
                    cv.Required(CONF_TYPE): cv.one_of(*MENU_TYPES, lower=True),
                    cv.Optional(CONF_MQTT_TOPIC): cv.string,
                    cv.Optional(CONF_MQTT_PAYLOAD): cv.string,
                    cv.Optional(CONF_READONLY, default=False): cv.boolean,
                    cv.Optional(CONF_CONDITION_TOPIC): cv.string,
                    cv.Optional(CONF_CONDITION_VALUE): cv.string,
                    cv.Optional(CONF_FILE_PATH): cv.string,
                    cv.Optional(CONF_MAX_ENTRIES, default=100): cv.positive_int,
                }),
                validate_menu_item
            )
        ),
        cv.Optional(CONF_MQTT_TOPIC): cv.string,
        cv.Optional(CONF_MQTT_PAYLOAD): cv.string,
        cv.Optional(CONF_READONLY, default=False): cv.boolean,
        cv.Optional(CONF_CONDITION_TOPIC): cv.string,
        cv.Optional(CONF_CONDITION_VALUE): cv.string,
        cv.Optional(CONF_FILE_PATH): cv.string,
        cv.Optional(CONF_MAX_ENTRIES, default=100): cv.positive_int,
    }),
    validate_menu_item
)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RobCoTerminal),
    cv.Optional(CONF_KEYBOARD_ID): cv.string,
    cv.Optional(CONF_USB_DP_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_USB_DM_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_DOWN_BUTTON_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_ENTER_BUTTON_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_BACK_BUTTON_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_MQTT_TOPIC_PREFIX, default="robco_terminal"): cv.string,
    cv.Optional(CONF_BOOT_SEQUENCE, default=True): cv.boolean,
    cv.Optional(CONF_CURSOR_BLINK, default=True): cv.boolean,
    cv.Optional(CONF_FONT_COLOR, default=0x00FF00): cv.hex_uint32_t,
    cv.Optional(CONF_BACKGROUND_COLOR, default=0x000000): cv.hex_uint32_t,
    cv.Optional(CONF_MENU_ITEMS, default=[]): cv.ensure_list(MENU_ITEM_SCHEMA),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Add Arduino_GFX library dependency
    cg.add_library("moononournation/GFX Library for Arduino", "1.6.0")
    
    # Set USB pins if configured
    if CONF_USB_DP_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_USB_DP_PIN])
        cg.add(var.set_usb_dp_pin(pin))
    if CONF_USB_DM_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_USB_DM_PIN])
        cg.add(var.set_usb_dm_pin(pin))
    
    # Set button pins if configured
    if CONF_DOWN_BUTTON_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_DOWN_BUTTON_PIN])
        cg.add(var.set_down_button_pin(pin))
    if CONF_ENTER_BUTTON_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_ENTER_BUTTON_PIN])
        cg.add(var.set_enter_button_pin(pin))
    if CONF_BACK_BUTTON_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_BACK_BUTTON_PIN])
        cg.add(var.set_back_button_pin(pin))
    
    # Set configuration
    cg.add(var.set_mqtt_topic_prefix(config[CONF_MQTT_TOPIC_PREFIX]))
    cg.add(var.set_boot_sequence(config[CONF_BOOT_SEQUENCE]))
    cg.add(var.set_cursor_blink(config[CONF_CURSOR_BLINK]))
    cg.add(var.set_font_color(config[CONF_FONT_COLOR]))
    cg.add(var.set_background_color(config[CONF_BACKGROUND_COLOR]))
    
    # Add menu items
    for item in config[CONF_MENU_ITEMS]:
        cg.add(var.add_menu_item(
            item[CONF_TITLE],
            item[CONF_TYPE],
            item.get(CONF_MQTT_TOPIC, ""),
            item.get(CONF_MQTT_PAYLOAD, ""),
            item.get(CONF_READONLY, False),
            item.get(CONF_CONDITION_TOPIC, ""),
            item.get(CONF_CONDITION_VALUE, ""),
            item.get(CONF_FILE_PATH, ""),
            item.get(CONF_MAX_ENTRIES, 100)
        ))
        
        # Add submenu items recursively
        if CONF_ITEMS in item:
            for subitem in item[CONF_ITEMS]:
                cg.add(var.add_submenu_item(
                    item[CONF_TITLE],
                    subitem[CONF_TITLE],
                    subitem[CONF_TYPE],
                    subitem.get(CONF_MQTT_TOPIC, ""),
                    subitem.get(CONF_MQTT_PAYLOAD, ""),
                    subitem.get(CONF_READONLY, False),
                    subitem.get(CONF_CONDITION_TOPIC, ""),
                    subitem.get(CONF_CONDITION_VALUE, ""),
                    subitem.get(CONF_FILE_PATH, ""),
                    subitem.get(CONF_MAX_ENTRIES, 100)
                ))
