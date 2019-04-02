# Old-style MLV level format (abandoned in favor of the modern TMX format)

Reading in obj-c:

```objective-c
-(void)loadTheLevel
{
    BinaryDataScanner *scanner = [BinaryDataScanner binaryDataScannerWithData:data littleEndian:NO defaultEncoding:NSASCIIStringEncoding];

    name = [[scanner readNullTerminatedString] retain];
    graphicsSet = [[scanner readNullTerminatedString] retain];
    infotrons = [scanner readWord];
    width = [scanner readWord];
    height = [scanner readWord];

    uint32_t array_size = width * height;
    iconS = malloc(sizeof(uint8_t) * array_size);
    iconT = malloc(sizeof(uint8_t) * array_size);

    uint8_t *current_s = iconS;
    uint8_t *current_t = iconT;

    for (uint16_t y = 0; y < height; y++)
    {
        for (uint16_t x = 0; x < width; x++)
        {
            *current_s = [scanner readByte]; current_s++;
            *current_t = [scanner readByte]; current_t++;
        }
    }
}
```
