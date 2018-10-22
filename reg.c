#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAP_SIZE 11*sizeof(unsigned int) //11 registers
#define MAP_MASK (MAP_SIZE - 1)

//Temp
#define BASE_ADDR    0x01C25000
#define CONTROL_0    0x00
#define CONTROL_1    0x04
#define CALIBRATION  0x14
#define CONTROL_2    0x40
#define INTERRUPT    0x44
#define STATUS       0x48
#define ALARM        0x50
#define SHUTDOWN     0x60
#define AVERAGE      0x70
#define THERMAL      0x74
#define DATA         0x80

#define MHZ          24
#define M            1
#define N            17
#define CLK_IN       MHZ/M
#define CONV         1/(MHZ/M/14)
#define ACQ1         CLK_IN/(N+1)
#define ACQ0         CLK_IN/(N+1)
#define THERMAL_PER  4096*(N+1)/CLK_IN // =3072 but ACQ0+ACQ1+CONV < 4



//CCU - PLL
#define CCU_BASE     0x01C20000
#define PLL_CPUX     0x0000
//PLL_FACTOR_M is used in some registers; we are assuming M=1; otherwise check PLL_CPUX register
//PLL_FACTOR_N is used in some registers; we are assuming N=17; otherwise check PLL_CPUX register

int main() {
  int mem;
  char status;
  void *map;

  /* Open /dev/mem */
  if ((mem = open ("/dev/mem", O_RDWR | O_SYNC)) == -1)
    fprintf(stderr, "Cannot open /dev/mem\n"), exit(1);

  if ((map = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem, BASE_ADDR & ~MAP_MASK)) == NULL)
    fprintf(stderr, "Cannot open Memory Map\n"), exit(1);

  volatile unsigned int *interrupt_reg = (volatile unsigned int *)(map + INTERRUPT);
  volatile unsigned int *control_0_reg = (volatile unsigned int *)(map + CONTROL_0);
  volatile unsigned int *control_1_reg = (volatile unsigned int *)(map + CONTROL_1);
  volatile unsigned int *control_2_reg = (volatile unsigned int *)(map + CONTROL_2);
  volatile unsigned int *alarm_reg = (volatile unsigned int *)(map + ALARM);
  volatile unsigned int *data_reg = (volatile unsigned int *)(map + DATA);
  
  printf("Little sanity test (this should reg 0x05A00684): %X\n", *alarm_reg);
  
  /* THERMAL_PER > ACQ1 + ACQ0 + Conversion Time */
  /* Configure THS Interrupt to set THERMAL_PER and IRQ */
  /* Configure Alarm threshold and Shutdown threshold to set ALARM_T_HOT and SHUT_T_HOT */
  /* Configure THS to set SENSOR_ACQ and enable sensor */
  /* The real temperature value is Tem, then T = (Tem - 2794) / -14.882 */
  /* Temp data is 12-bit */
  
  printf("THERMAL_PER: %X \nACQ1: %X \nACQ0: %X \n", *interrupt_reg >> 12, *control_2_reg >> 16, *control_0_reg & 0xF0); 
  *interrupt_reg |= (THERMAL_PER << 12);
  *control_2_reg |= (ACQ1 << 16);
  *control_0_reg |= (ACQ0);
  printf("THERMAL_PER: %X \nACQ1: %X \nACQ0: %X \n", *interrupt_reg >> 12, *control_2_reg >> 16, *control_0_reg & 0xF0); 
  
  printf("%X\n", *control_2_reg);
  *control_2_reg |= 1;
  printf("%X\n", *control_2_reg);
  
  for(;;) {
    printf("%X\n", *data_reg);
    sleep(1);
  }

  /* close all registers */
  if (munmap(map, MAP_SIZE) == -1)
    fprintf(stderr, "Cannot close Memory Map"), exit(1);

  /* close /dev/mem */
  close(mem);

  return 0;
}
