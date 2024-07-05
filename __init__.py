import esphome.config_validation as cv
import esphome.codegen as cg

from esphome.const import CONF_ID

CODEOWNERS = ["@mangavalk"]
MULTI_CONF = True

CONF_ONKYO_AUDIO = "onkyo_audio"
CONF_ONKYO_AUDIO_ID = "onkyo_audio_id"

onkyo_audio_ns = cg.esphome_ns.namespace("onkyo_audio")
OnkyoAudioComponent = onkyo_audio_ns.class_("OnkyoAudioComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(OnkyoAudioComponent)
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
