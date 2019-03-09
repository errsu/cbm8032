package tiff2c

import javax.imageio.ImageReader
import java.io.File
import com.sun.media.jai.codec.ImageCodec;
import com.sun.media.jai.codec.TIFFDecodeParam;

fun main(args : Array<String>) {
    val fileName = "E:/dev/Robert/errsu-cbm8032/doc/PetASCII.tif"
    val readers = javax.imageio.ImageIO.getImageReadersBySuffix("tiff")

    if (readers.hasNext()) {

        val fi = File(fileName)

        val iis = javax.imageio.ImageIO.createImageInputStream(fi)

        val param: TIFFDecodeParam? = null
        val dec = ImageCodec.createImageDecoder("tiff", fi, param)

        val pageCount = dec.getNumPages()

        val _imageReader = readers.next() as ImageReader

        if (_imageReader != null) {

            _imageReader.setInput(iis, true)

            for (i in 0 until pageCount) {

                val bufferedImage = _imageReader.read(i)
                println("unsigned char petASCIIImage[] = {")

                val width = bufferedImage.width
                val height = bufferedImage.height
                for (y in height - 1 downTo 0) {
                    if (y % 25 == 24) {
                        println("  // line ${(25 * 16 - y) / 25}")
                    } else {
                        var accumulatedBits = 0
                        var bitCount = 0
                        for (x in 0 until width) {
                            if (x % 17 != 16) {
                                val rgb = bufferedImage.getRGB(x, y).and(0x00FFFFFF)
                                val bit = if (rgb == 0) 0 else 1
                                accumulatedBits = accumulatedBits.or(bit.shl(bitCount))
                                bitCount += 1
                                if (bitCount == 8) {
                                    print("0x%02x, ".format(accumulatedBits))
                                    accumulatedBits = 0
                                    bitCount = 0
                                }
                            }
                        }
                        println()
                    }
                }
                println("};")
            }
        }
    }
}
