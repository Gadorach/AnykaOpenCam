/* Exact owned retail MCU firmware data, not host/main-CPU executable code.
 * Firmware licensing unchanged/unspecified; local testing only, no publication.
 */
extern const unsigned char radio_payload_begin[],radio_payload_end[];
__asm__(".pushsection .rodata.radio_payload,\"a\"\n.balign 4\n.global radio_payload_begin\nradio_payload_begin:\n.incbin \"/home/gadorach/Desktop/anyka-km01-agent-handoff/work/lightbulb-camera/implementation-20261008T205404Z/artifacts/retail-radio-payload36.bin\"\n.global radio_payload_end\nradio_payload_end:\n.popsection\n");
