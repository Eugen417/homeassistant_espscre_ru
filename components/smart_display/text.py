import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text
from esphome.const import CONF_ID

AUTO_LOAD = ["json"]

ns = cg.esphome_ns.namespace("smart_display")
DashboardInbox = ns.class_("DashboardInbox", text.Text, cg.Component)
CONFIG_SCHEMA = text.text_schema(DashboardInbox, mode="TEXT", entity_category="config").extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    # A tap asks Home Assistant whether it took the action (firmware 0.2.58+): ESPHome's own action responses, which it
    # otherwise only compiles in for a YAML homeassistant.action with on_success or on_error.
    cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await text.register_text(var, config, min_length=0, max_length=255, pattern=None)
