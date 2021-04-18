#ifndef DATAPOINT_H
#define DATAPOINT_H

struct DataPoint {
	String name;
	String unit;
	float val;
	float (*valfunc)();
};

#endif
