package tiff2c

import java.awt.image.BufferedImage
import javax.imageio.ImageReader
import sun.awt.image.ImageDecoder
import javax.imageio.stream.ImageInputStream
import java.io.File
import com.sun.media.jai.codec.ImageCodec;
import com.sun.media.jai.codec.TIFFDecodeParam;

fun main(args : Array<String>) {
    println("Hello, world!")

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
                println("unsigned char image[] = {")

                val width = bufferedImage.width
                val height = bufferedImage.height
                for (y in 0 until height) {
                    if (y % 25 == 0) {
                        println("  // line ${y / 25}")
                    }
                    if (y % 25 != 24) {
                        for (x in 0 until width) {
                            if (x % 17 == 0) {
                                print("  ")
                            }
                            val rgb = bufferedImage.getRGB(x, y).and(0x00FFFFFF)
                            val c = if (rgb == 0) '0' else if (rgb == 0x00FFFFFF) '1' else '?'
                            if (x % 17 == 16) {
                                // println()
                            } else {
                                print("$c, ")
                            }
                        }
                        println()
                    }
                }
                println("}")
            }
        }
    }
}
