import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome.components import uart

from esphome.const import CONF_ID

from .. import (
    onkyo_audio_ns,
    OnkyoAudioComponent
)

CODEOWNERS = ["@mangavalk"]
DEPENDENCIES = ["uart"]

CONF_UART = "uart"

OnkyoAudioMediaPlayer = onkyo_audio_ns.class_(
    "OnkyoAudioMediaPlayer", cg.Component, media_player.MediaPlayer
)

CONFIG_SCHEMA = cv.All(
    cv.typed_schema(
        {
            "internal": media_player.MEDIA_PLAYER_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(OnkyoAudioMediaPlayer),
                    cv.Required(CONF_UART): cv.use_id(uart.UARTComponent),
                }
            ).extend(cv.COMPONENT_SCHEMA),
            "external": media_player.MEDIA_PLAYER_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(OnkyoAudioMediaPlayer),
                    cv.Required(CONF_UART): cv.use_id(uart.UARTComponent),
                }
            ).extend(cv.COMPONENT_SCHEMA),
        },
    ),
)



async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await media_player.register_media_player(var, config)
    
    uart = await cg.get_variable(config[CONF_UART])
    cg.add(var.set_uart(uart))
