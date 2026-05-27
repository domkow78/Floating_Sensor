/*
 * comp1.h
 *
 *  Created on: 18 wrz 2020
 *      Author: JablonskiP
 */

#ifndef COMPONENTS_COMP1_COMP1_H_
#define COMPONENTS_COMP1_COMP1_H_

typedef struct {
	unsigned int d;
	double intercept;
	double sigma_2;
} comp1struct;

class comp1class{
public:
	int a;
	int getA(void);
};





#endif /* COMPONENTS_COMP1_COMP1_H_ */
