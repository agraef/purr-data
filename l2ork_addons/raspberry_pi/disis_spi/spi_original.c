/***********************************************************************
 * This header file contains the mcp3008Spi class definition.
 * Its main purpose is to communicate with the MCP3008 chip using
 * the userspace spidev facility.
 * The class contains four variables:
 * mode        -> defines the SPI mode used. In our case it is SPI_MODE_0.
 * bitsPerWord -> defines the bit width of the data transmitted.
 *        This is normally 8. Experimentation with other values
 *        didn't work for me
 * speed       -> Bus speed or SPI clock frequency. According to
 *                https://projects.drogon.net/understanding-spi-on-the-raspberry-pi/
 *            It can be only 0.5, 1, 2, 4, 8, 16, 32 MHz.
 *                Will use 1MHz for now and test it further.
 * spifd       -> file descriptor for the SPI device
 *
 * The class contains two constructors that initialize the above
 * variables and then open the appropriate spidev device using spiOpen().
 * The class contains one destructor that automatically closes the spidev
 * device when object is destroyed by calling spiClose().
 * The spiWriteRead() function sends the data "data" of length "length"
 * to the spidevice and at the same time receives data of the same length.
 * Resulting data is stored in the "data" variable after the function call.
 * ****************************************************************************/
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
     
typedef struct _spi {
    unsigned char mode;
    unsigned char bitsPerWord;
    unsigned int speed;
    int spifd;     
} t_spi;

t_spi *spi_new(const char *devspi, unsigned char spiMode, unsigned int spiSpeed, unsigned char spibitsPerWord);
int spi_WriteRead(t_spi *spi, unsigned char *data, int length);
int spi_Open(t_spi *spi, const char *devspi);
int spi_Close(t_spi *spi);

//using namespace std;
/**********************************************************
 * spiOpen() :function is called by the constructor.
 * It is responsible for opening the spidev device
 * "devspi" and then setting up the spidev interface.
 * private member variables are used to configure spidev.
 * They must be set appropriately by constructor before calling
 * this function.
 * *********************************************************/
int spi_Open(t_spi *spi, const char *devspi){
    int statusVal = -1;
    spi->spifd = open(devspi, O_RDWR);
    if(spi->spifd < 0) {
      perror("could not open SPI device");
      exit(1);
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_WR_MODE, &(spi->mode));
    if(statusVal < 0){
      perror("Could not set SPIMode (WR)...ioctl fail");
      exit(1);
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_RD_MODE, &(spi->mode));
    if(statusVal < 0) {
      perror("Could not set SPIMode (RD)...ioctl fail");
      exit(1);
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_WR_BITS_PER_WORD, &(spi->bitsPerWord));
    if(statusVal < 0) {
      perror("Could not set SPI bitsPerWord (WR)...ioctl fail");
      exit(1);
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_RD_BITS_PER_WORD, &(spi->bitsPerWord));
    if(statusVal < 0) {
      perror("Could not set SPI bitsPerWord(RD)...ioctl fail");
      exit(1);
    }  
 
    statusVal = ioctl (spi->spifd, SPI_IOC_WR_MAX_SPEED_HZ, &(spi->speed));    
    if(statusVal < 0) {
      perror("Could not set SPI speed (WR)...ioctl fail");
      exit(1);
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_RD_MAX_SPEED_HZ, &(spi->speed));    
    if(statusVal < 0) {
      perror("Could not set SPI speed (RD)...ioctl fail");
      exit(1);
    }
    return statusVal;
}
 
/***********************************************************
 * spiClose(): Responsible for closing the spidev interface.
 * Called in destructor
 * *********************************************************/
 
int spi_free(t_spi *spi){
    int statusVal = -1;
    statusVal = close(spi->spifd);
        if(statusVal < 0) {
      perror("Could not close SPI device");
      exit(1);
    }
    return statusVal;
}
 
/********************************************************************
 * This function writes data "data" of length "length" to the spidev
 * device. Data shifted in from the spidev device is saved back into
 * "data".
 * ******************************************************************/
int spi_WriteRead(t_spi *spi, unsigned char *data, int length){
 
  struct spi_ioc_transfer spid[length];
  int i = 0;
  int retVal = -1; 
 
// one spi transfer for each byte
 
  for (i = 0 ; i < length ; i++){
 
    spid[i].tx_buf        = (unsigned long)(data + i); // transmit from "data"
    spid[i].rx_buf        = (unsigned long)(data + i); // receive into "data"
    spid[i].len           = sizeof(*(data + i));
    spid[i].delay_usecs   = 0;
    spid[i].speed_hz      = spi->speed;
    spid[i].bits_per_word = spi->bitsPerWord;
    spid[i].cs_change = 0;
  }
 
  retVal = ioctl(spi->spifd, SPI_IOC_MESSAGE(length), &spid);

  if(retVal < 0){
    perror("Problem transmitting spi data..ioctl");
    exit(1);
  }

  return retVal;
}
 
/*************************************************
 * Default constructor. Set member variables to
 * default values and then call spiOpen()
 * ***********************************************/
/* 
t_spi *spi_new(){
    spi->mode = SPI_MODE_0 ;
    spi->bitsPerWord = 8;
    spi->speed = 1000000;
    spi->spifd = -1;
 
    spi->spiOpen(std::string("/dev/spidev0.0"));
 
    }
 */
/*************************************************
 * overloaded constructor. let user set member variables to
 * and then call spiOpen()
 * ***********************************************/
t_spi *spi_new(const char *devspi, unsigned char spiMode, unsigned int spiSpeed, unsigned char spibitsPerWord){
    t_spi *spi = (t_spi *)malloc(sizeof(t_spi)); 
    spi->mode = spiMode;
    spi->bitsPerWord = spibitsPerWord;
    spi->speed = spiSpeed;
    spi->spifd = -1;
 
    spi_Open(spi, devspi);
    return spi;
}

/***********************************************************************
 * mcp3008SpiTest.cpp. Sample program that tests the mcp3008Spi class.
 * an mcp3008Spi class object (a2d) is created. the a2d object is instantiated
 * using the overloaded constructor. which opens the spidev0.0 device with
 * SPI_MODE_0 (MODE 0) (defined in linux/spi/spidev.h), speed = 1MHz &
 * bitsPerWord=8.
 *
 * call the spiWriteRead function on the a2d object 20 times. Each time make sure
 * that conversion is configured for single ended conversion on CH0
 * i.e. transmit ->  byte1 = 0b00000001 (start bit)
 *                   byte2 = 0b1000000  (SGL/DIF = 1, D2=D1=D0=0)
 *                   byte3 = 0b00000000  (Don't care)
 *      receive  ->  byte1 = junk
 *                   byte2 = junk + b8 + b9
 *                   byte3 = b7 - b0
 *    
 * after conversion must merge data[1] and data[2] to get final result
 *
 *
 *
 * *********************************************************************/
//using namespace std;
 
int main(void)
{
  t_spi *a2d = spi_new("/dev/spidev0.0", SPI_MODE_0, 1000000, 8);
  int a2dVal = 0;
  int a2dChannel = 0;
  unsigned char data[3];

  while(1)
  {
    data[0] = 1;  //  first byte transmitted -> start bit
    data[1] = 0b10000000 |( ((a2dChannel & 7) << 4)); // second byte transmitted -> (SGL/DIF = 1, D2=D1=D0=0)
    data[2] = 0; // third byte transmitted....don't care

    spi_WriteRead(a2d, data, sizeof(data));

    a2dVal = 0;
    a2dVal = (data[1]<< 8) & 0b1100000000; //merge data[1] & data[2] to get result
    a2dVal |=  (data[2] & 0xff);
    usleep(10);
    fprintf(stderr,"%d\n", a2dVal);
  }
  return 0;
}
