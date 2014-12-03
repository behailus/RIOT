/*
 * Rshell.h
 *
 *  Created on: Nov 25, 2014
 *      Author: behailus
 */

#ifndef RSHELL_H_
#define RSHELL_H_

//Method definition for shell commands created for this application
void nesb_send_ping(int argc, char **argv);

void nesb_app_commands(int argc, char **argv);

void nesb_service(int argc, char **argv);

void nesb_manager(int argc, char **argv);

#endif /* RSHELL_H_ */
