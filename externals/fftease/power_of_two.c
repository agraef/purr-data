
int fftease_power_of_two(int test)
{
	int limit = 1048576;
	int compare = 1;
	do {
		if(test == compare){
			return 1;
		} 
		compare *= 2;
	} while (compare <= limit);
	
	return 0;
}

