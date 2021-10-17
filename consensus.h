#ifndef CONSENSUS_H
#define CONSENSUS_H

class admm{

public:
	float* d;
	float* d_;
	float* y;
	int max_iterations = 50;
	float o, L;
	int upper_bound;
	float rho = 0.03;

	admm(int, int); // ctor 
  void init(int);
	void update(float, float*, float, float);
	void argmin();

private:
  float tol = 0.001;
  
	int indice, N;
	float* c;
	float* k;
	float* z;

  float* d_aux;
	float** A;
	float b[3];

	void compute_z();
	float multiply_array(float* a, float* b);
	void df();
	void ILB();
	void DLB();
	void DUB();
	void ILB_DLB();
	void ILB_DUB();
	bool is_feasible(float* x);
	float compute_cost(float* x);
};

#endif //CONSENSUS_H
