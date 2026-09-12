import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text
from esphome.const import CONF_ID

AUTO_LOAD = ["json"]

ns = cg.esphome_ns.namespace("smart_display")
DashboardInbox = ns.class_("DashboardInbox", text.Text, cg.Component)
CONFIG_SCHEMA = text.text_schema(DashboardInbox, mode="TEXT", entity_category="config").extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await text.register_text(var, config, min_length=0, max_length=255, pattern=None)
