import esphome.codegen as cg
from esphome.components import media_player, uart
import esphome.config_validation as cv

from .. import onkyo_audio_ns

CODEOWNERS = ["@MangaValk"]
DEPENDENCIES = ["uart"]

CONF_MAX_VOLUME = "max_volume"
CONF_UART = "uart"  # legacy alias for uart_id
CONF_TYPE = "type"  # legacy option, ignored

OnkyoAudioMediaPlayer = onkyo_audio_ns.class_(
    "OnkyoAudioMediaPlayer",
    media_player.MediaPlayer,
    cg.PollingComponent,
    uart.UARTDevice,
)

CONFIG_SCHEMA = (
    media_player.media_player_schema(OnkyoAudioMediaPlayer)
    .extend(
        {
            # Highest MVL value of the receiver; Home Assistant's 0-100 % maps onto 0..max_volume.
            cv.Optional(CONF_MAX_VOLUME, default=78): cv.int_range(min=1, max=200),
            # Kept so configurations written for the first version keep validating.
            cv.Optional(CONF_UART): cv.use_id(uart.UARTComponent),
            cv.Optional(CONF_TYPE): cv.one_of("internal", "external", lower=True),
        }
    )
    .extend(cv.polling_component_schema("30s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "onkyo_audio", baud_rate=9600, require_tx=True, require_rx=True
)


async def to_code(config):
    var = await media_player.new_media_player(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    if CONF_UART in config:
        cg.add(var.set_uart_parent(await cg.get_variable(config[CONF_UART])))
    cg.add(var.set_max_volume(config[CONF_MAX_VOLUME]))
