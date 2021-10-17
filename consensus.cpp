#include <stdlib.h>
#include "consensus.h" 
using namespace std;

admm::admm(int ID, int max_value){
	indice = ID - 1;
	upper_bound = max_value;
}

void admm::init(int number_elements){
  
  c = (float*) calloc (number_elements, sizeof(float));
  k = (float*) calloc (number_elements, sizeof(float));

  d = (float*) calloc (number_elements, sizeof(float));
  d_ = (float*) calloc (number_elements, sizeof(float));
  d_aux = (float*) calloc (number_elements, sizeof(float));
  y = (float*) calloc (number_elements, sizeof(float));
  z = (float*) calloc (number_elements, sizeof(float));

  A = (float**) calloc (3, sizeof(float*));
  for (int i = 0; i < 3; ++i)
    A[i] = (float*) calloc (number_elements, sizeof(float));

  N = number_elements;
}

void admm::update(float cost, float* coefficients, float error, float objective){

	c[indice] = cost;
	o = error;
	L = objective;

	for(int i = 0; i < N; ++i){
		k[i] = coefficients[i];
		A[0][i] = -k[i];
		A[1][i] = -(i == indice);
		A[2][i] = -A[1][i];
	}

	b[0] = o - L;
	b[1] = 0;
	b[2] = upper_bound;

	for(int i = 0; i < N; ++i){
		d[i] = 0; 
		d_[i] = 0;
		y[i] = 0;
	}

}

void admm::compute_z(){
	for(int i = 0; i < N; ++i)
		z[i] = rho*d_[i] - c[i] - y[i];
}

float admm::multiply_array(float* a, float* b){
		
	float value = 0;
	for(int i = 0; i < N; ++i)
		value += a[i]*b[i];

	return value;
}

void admm::df(){
	for(int i = 0; i < N; ++i)
		d[i] = z[i]/rho;
}

// The illuminance lower bound
void admm::ILB(){
	float norm_k = multiply_array(k,k);
	float k_times_z = multiply_array(k,z);

	for(int i = 0; i < N; ++i)
		d_aux[i] = z[i]/rho - k[i]/norm_k*(o - L + k_times_z/rho);
}

// The lower bound on its dimming level
void admm::DLB(){
	for(int i = 0; i < N; ++i)
		d_aux[i] = z[i]/rho;

	d_aux[indice] = 0; 
}

// The upper bound on its dimming level
void admm::DUB(){
	for(int i = 0; i < N; ++i)
		d_aux[i] = z[i]/rho;

	d_aux[indice] = upper_bound; 
}

// The intersection between the illuminance lower bound and the lower bound on its dimming level
void admm::ILB_DLB(){
	float numerator = k[indice]*z[indice] - multiply_array(k,z);
	float denominator = multiply_array(k,k) - k[indice]*k[indice];

	for(int i = 0; i < N; ++i)
		d_aux[i] = z[i]/rho - k[i]/denominator*(o - L)  - k[i]/rho/denominator*numerator;
	
	d_aux[indice] = 0;
}

// The intersection between the illuminance lower bound and the upper bound on its dimming level
void admm::ILB_DUB(){
	float numerator = k[indice]*z[indice] - multiply_array(k,z);
	float denominator = multiply_array(k,k) - k[indice]*k[indice];

	for(int i = 0; i < N; ++i)
		d_aux[i] = z[i]/rho - k[i]/denominator*(o - L + upper_bound*k[indice])  - k[i]/rho/denominator*numerator;
	
	d_aux[indice] = upper_bound;
}

bool admm::is_feasible(float* x){

	for(int i = 0; i < 3; ++i)
		if(multiply_array(A[i],x) > b[i] + tol)
			return false;

	return true;
}

float admm::compute_cost(float* x){
  float cost = 0;
  float dif;
  for(int i = 0; i < N; ++i){
    dif = x[i] - d_[i];
    cost += c[i]*x[i] + y[i]*dif + rho/2*dif*dif;
  }
	return cost;
}

void admm::argmin(){

	float cost = 100000;

	compute_z();
	df();
	if(!is_feasible(d)){
		for(int i = 0; i < 5; ++i){
			switch(i){
				case 0:
					ILB();
					break;

				case 1:
					DLB();
					break;

				case 2:
					DUB();
					break;

				case 3:
					ILB_DLB();
					break;

				case 4:
					ILB_DUB();
					break;
			}
			if(is_feasible(d_aux) && cost > compute_cost(d_aux)){
				cost = compute_cost(d_aux);
				for(int i = 0; i < N; ++i)
					d[i] = d_aux[i];
			}
		}
	}
	
	for(int i = 0; i < N; ++i)
		d_[i] = d[i];
}
