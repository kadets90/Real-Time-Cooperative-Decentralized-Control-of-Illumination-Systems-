#ifndef PID_H
#define PID_H

class pid{

	bool derivative;
	float kp, ki, kd, T, a;
	float k1, k2, k3, k4;
	float ep, yp, ip, dp;
	float umin = -100.0, umax = 420.0;

public:
	pid(float kp, float ki, float kd, float T, float a); // ctor
	float calc(float ref, float y, float uff);
};

#endif //PID_H
