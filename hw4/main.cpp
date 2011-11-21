#include "matrix.h"
#include "mbed.h"
#include "scheduler.h"
#include "sdfg.h"
#include "simulator.h"

#define SDFG_FILENAME "/local/sdfgconf.txt"
#define INPUT_FILENAME "/local/input.txt"
#define OUTPUT_FILENAME "/local/output.txt"

LocalFileSystem local("local");
Serial pc(USBTX, USBRX);
Timer global_timer;

DigitalOut led_1(LED1);
DigitalOut pin_21(LED2);
DigitalOut pin_22(LED3);
DigitalOut pin_23(LED4);

signed long ext_mem0[4096] __attribute__ ((section("AHBSRAM0")));
signed long ext_mem1[4096] __attribute__ ((section("AHBSRAM1")));

int main() {
  global_timer.start();
  
  int timestamp_us;
  
  led_1 = 0;
  pin_21 = 0;
  pin_22 = 0;
  pin_23 = 0;
  
  struct Sdfg *sdfg = NULL;
  struct Matrix *m = NULL;
  struct Schedule *s = NULL;
  struct Simulator *sim = NULL;
  
  FILE *sdfgconf_fp = NULL;
  FILE *input_fp = NULL;
  FILE *output_fp = NULL;
  
  sdfgconf_fp = fopen(SDFG_FILENAME, "r");
  if (sdfgconf_fp == NULL) {
    error("NO SDFG");
    goto on_error;
  }
  pc.printf("BEGIN SDFG READ\n\r");
  sdfg = create_sdfg_from_file(sdfgconf_fp);
  fclose(sdfgconf_fp);
  if (sdfg == NULL) {
    error("BAD SDFG FORMAT\n\r");
    goto on_error;
  }
  pc.printf("NODES: %d EDGES: %d\n\r", sdfg->num_nodes, sdfg->num_edges);
  
  pc.printf("LOADING MATRIX FROM SDFG:\n\r");
  m = generate_matrix_from_sdfg(sdfg);
  pc.printf("LOADED MATRIX FROM SDFG\n\r");
  print_matrix(m);
  pc.printf("ROW REDUCING MATRIX\n\r");
  matrix_to_rref(m);
  pc.printf("ROW REDUCED MATRIX:\n\r");
  print_matrix(m);

  s = init_schedule(sdfg->num_nodes, m);
  if (s == NULL) {
    error("CANNOT SCHEDULE");
    goto on_error;
  }
  pc.printf("SCHEDULING SDFG\n\r");
  schedule_sdfg(sdfg, s);
  print_schedule(sdfg, s);
  free_matrix(m);
  
  // do blinkenlights
  led_1 = 1;
  pin_21 = 1;
  pc.printf("READY @ %d ms\n\r", global_timer.read_ms());
  
  input_fp = fopen(INPUT_FILENAME, "r");
  if (input_fp == NULL) {
    error("NO INPUT");
    goto on_error;
  }
  
  // do more blinkenlights
  pin_22 = 1;
  
  pc.printf("BEGIN INPUT.TXT LOAD\n\r");
  sim = init_simulator_from_file((int *) &ext_mem0, (int *) &ext_mem1, input_fp);
  fclose(input_fp);
  if (sim == NULL) {
    error("BAD INPUT FORMAT");
    goto on_error;
  }
  pc.printf("LOADED INPUT.TXT\n\r");
  
  // even more blinkenlights
  pin_23 = 1;
  
  print_simulator(sim);
  pc.printf("BEGINNING SIMULATION\n\r");
  timestamp_us = global_timer.read_us();
  simulate(sim, sdfg, s);
  timestamp_us = global_timer.read_us() - timestamp_us;
  pc.printf("SIMULATION ENDED\n\r");
  
  // penultimate blinkenlights
  pin_23 = 0;
  
  pc.printf("BEGIN OUTPUT.TXT WRITE\n\r");
  output_fp = fopen(OUTPUT_FILENAME, "w");
  if (output_fp == NULL) {
    error("UNABLE TO CREATE OUTPUT.TXT\n\r");
    goto on_error;
  }
  write_output(sim, output_fp);
  fclose(output_fp);
  pc.printf("FINISHED WRITE TO OUTPUT.TXT\n\r");
  
  // last blinkenlights
  pc.printf("DONE: %d 0x%x %d\n\r", sim->iterations * sim->num_samples, sim->checksum, timestamp_us);
  pin_22 = 0;
  pc.printf("COMPLETE\n\r");

  free_simulator(sim);
  free_schedule(s);
  free_sdfg(sdfg);
  return 0;
  
on_error:
  fclose(output_fp);
  fclose(input_fp);
  fclose(sdfgconf_fp);
  free_simulator(sim);
  free_schedule(s);
  free_matrix(m);
  free_sdfg(sdfg);
  return -1;
}
