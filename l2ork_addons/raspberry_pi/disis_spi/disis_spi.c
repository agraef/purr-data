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
 * ****************************************************************************/
#include "m_pd.h"
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static t_class *disis_spi_class;

typedef struct _disis_spi
{
    t_object x_obj;
    t_outlet *x_out1;
    t_outlet *x_out2;
    t_outlet *x_out3;
    t_outlet *x_out4;
    t_outlet *x_out5;
    t_outlet *x_out6;
    t_outlet *x_out7;
    t_outlet *x_out8;
    t_outlet *x_out9;
    t_symbol *spidev;
    unsigned char mode;
    unsigned char bitsPerWord;
    unsigned int speed;
    int spifd;
} t_disis_spi;

static t_disis_spi *disis_spi_new(t_symbol *devspi);
static int disis_spi_write_read(t_disis_spi *spi, unsigned char *data, int length);
static void disis_spi_open(t_disis_spi *spi, t_symbol *devspi);
static int disis_spi_close(t_disis_spi *spi);
static void disis_spi_free(t_disis_spi *spi);

/**********************************************************
 * disis_spi_open() :function is called by the "open" command
 * It is responsible for opening the spidev device
 * "devspi" and then setting up the spidev interface.
 * member variables are used to configure spidev.
 * They must be set appropriately before calling
 * this function.
 * *********************************************************/
static void disis_spi_open(t_disis_spi *spi, t_symbol *devspi){
    int statusVal = 0;
    if (strlen(devspi->s_name) == 0) {
      spi->spidev = gensym("/dev/spidev0.0");
    } else {
      spi->spidev = devspi;
    }
    spi->spifd = open(spi->spidev->s_name, O_RDWR);
    if(spi->spifd < 0) {
      statusVal = -1;
      pd_error(spi, "could not open SPI device");
      goto spi_output;
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_WR_MODE, &(spi->mode));
    if(statusVal < 0){
      pd_error(spi, "Could not set SPIMode (WR)...ioctl fail");
      disis_spi_close(spi);
      goto spi_output;
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_RD_MODE, &(spi->mode));
    if(statusVal < 0) {
      pd_error(spi, "Could not set SPIMode (RD)...ioctl fail");
      disis_spi_close(spi);
      goto spi_output;
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_WR_BITS_PER_WORD, &(spi->bitsPerWord));
    if(statusVal < 0) {
      pd_error(spi, "Could not set SPI bitsPerWord (WR)...ioctl fail");
      disis_spi_close(spi);
      goto spi_output;
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_RD_BITS_PER_WORD, &(spi->bitsPerWord));
    if(statusVal < 0) {
      pd_error(spi, "Could not set SPI bitsPerWord(RD)...ioctl fail");
      disis_spi_close(spi);
      goto spi_output;
    }  
 
    statusVal = ioctl (spi->spifd, SPI_IOC_WR_MAX_SPEED_HZ, &(spi->speed));    
    if(statusVal < 0) {
      pd_error(spi, "Could not set SPI speed (WR)...ioctl fail");
      disis_spi_close(spi);
      goto spi_output;
    }
 
    statusVal = ioctl (spi->spifd, SPI_IOC_RD_MAX_SPEED_HZ, &(spi->speed));    
    if(statusVal < 0) {
      pd_error(spi, "Could not set SPI speed (RD)...ioctl fail");
      disis_spi_close(spi);
      goto spi_output;
    }
spi_output:
    if (!statusVal) statusVal = 1;
    else statusVal = 0;
    outlet_float(spi->x_out9, statusVal);
}

/***********************************************************
 * disis_spi_close(): Responsible for closing the spidev interface.
 * *********************************************************/
 
static int disis_spi_close(t_disis_spi *spi){
    int statusVal = -1;
    if (spi->spifd == -1) {
      pd_error(spi, "disis_spi: device not open");
      return(-1);
    }
    statusVal = close(spi->spifd);
    if(statusVal < 0) {
      pd_error(spi, "disis_spi: could not close SPI device");
      exit(1);
    }
    outlet_float(spi->x_out9, 0);
    spi->spifd = -1;
    return(statusVal);
}

/********************************************************************
 * This function frees the object (destructor).
 * ******************************************************************/
static void disis_spi_free(t_disis_spi *spi){
    if (spi->spifd == 0) {
      disis_spi_close(spi);
    }
}
 
/********************************************************************
 * This function writes data "data" of length "length" to the spidev
 * device. Data shifted in from the spidev device is saved back into
 * "data".
 * ******************************************************************/
static int disis_spi_write_read(t_disis_spi *spi, unsigned char *data, int length){
 
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
    spid[i].cs_change     = 0;
  }
 
  retVal = ioctl(spi->spifd, SPI_IOC_MESSAGE(length), &spid);

  if(retVal < 0){
    pd_error(spi, "problem transmitting spi data..ioctl");
  }

  return retVal;
}

/***********************************************************************
 * mcp3008 enabled external that by default interacts with /dev/spidev0.0 device using
 * disis_spi_MODE_0 (MODE 0) (defined in linux/spi/spidev.h), speed = 1MHz &
 * bitsPerWord=8.
 *
 * on bang call the spi_write_read function on the a2d object and make sure
 * that conversion is configured for single ended conversion on CH0
 * i.e. transmit ->  byte1 = 0b00000001 (start bit)
 *                   byte2 = 0b1000000  (SGL/DIF = 1, D2=D1=D0=0)
 *                   byte3 = 0b00000000  (Don't care)
 *      receive  ->  byte1 = junk
 *                   byte2 = junk + b8 + b9
 *                   byte3 = b7 - b0
 *    
 * after conversion must merge data[1] and data[2] to get final result
 * *********************************************************************/
 
static void disis_spi_bang(t_disis_spi *spi)
{
  if (spi->spifd == -1) {
    pd_error(spi, "device not open %d", spi->spifd);
    return;
  }
  int a2dVal[8];
  int a2dChannel = 0;
  unsigned char data[3];

  for (a2dChannel = 0; a2dChannel < 8; a2dChannel++) {

    data[0] = 1;  //  first byte transmitted -> start bit
    data[1] = 0b10000000 |( ((a2dChannel & 7) << 4)); // second byte transmitted -> (SGL/DIF = 1, D2=D1=D0=0)
    data[2] = 0; // third byte transmitted....don't care

    disis_spi_write_read(spi, data, sizeof(data));

    a2dVal[a2dChannel] = 0;
    a2dVal[a2dChannel] = (data[1]<< 8) & 0b1100000000; //merge data[1] & data[2] to get result
    a2dVal[a2dChannel] |=  (data[2] & 0xff);
    //fprintf(stderr,"%d\n", a2dVal);
  }

  outlet_float(spi->x_out8, a2dVal[7]);
  outlet_float(spi->x_out7, a2dVal[6]);
  outlet_float(spi->x_out6, a2dVal[5]);
  outlet_float(spi->x_out5, a2dVal[4]);
  outlet_float(spi->x_out4, a2dVal[3]);
  outlet_float(spi->x_out3, a2dVal[2]);
  outlet_float(spi->x_out2, a2dVal[1]);
  outlet_float(spi->x_out1, a2dVal[0]);
}

/*************************************************
 * init function.
 * ***********************************************/
static t_disis_spi *disis_spi_new(t_symbol *devspi){
    t_disis_spi *spi = (t_disis_spi *)pd_new(disis_spi_class);
    //fprintf(stderr,"devspi<%s>\n", devspi->s_name); 
    //t_disis_spi *a2d = disis_spi_new("/dev/spidev0.0", spi_MODE_0, 1000000, 8);
    spi->x_out1 = outlet_new(&spi->x_obj, gensym("float"));
    spi->x_out2 = outlet_new(&spi->x_obj, gensym("float"));
    spi->x_out3 = outlet_new(&spi->x_obj, gensym("float"));
    spi->x_out4 = outlet_new(&spi->x_obj, gensym("float"));
    spi->x_out5 = outlet_new(&spi->x_obj, gensym("float"));
    spi->x_out6 = outlet_new(&spi->x_obj, gensym("float"));
    spi->x_out7 = outlet_new(&spi->x_obj, gensym("float"));
    spi->x_out8 = outlet_new(&spi->x_obj, gensym("float"));
    spi->x_out9 = outlet_new(&spi->x_obj, gensym("float"));
    spi->spidev = devspi;
    spi->mode = SPI_MODE_0;
    spi->bitsPerWord = 8;
    spi->speed = 1000000;
    spi->spifd = -1;
 
    return(spi);
}


void disis_spi_setup(void)
{
    disis_spi_class = class_new(gensym("disis_spi"), (t_newmethod)disis_spi_new,
        (t_method)disis_spi_free, sizeof(t_disis_spi), 0, A_DEFSYM, 0);
    class_addmethod(disis_spi_class, (t_method)disis_spi_open, gensym("open"), 
        A_DEFSYM, 0);
    class_addmethod(disis_spi_class, (t_method)disis_spi_close, gensym("close"), 
        0, 0);
    //class_addfloat(disis_gpio_class, disis_gpio_float); (later do sending data back to the spi)
    class_addbang(disis_spi_class, disis_spi_bang);
}
