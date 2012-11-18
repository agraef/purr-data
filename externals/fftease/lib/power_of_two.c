
int power_of_two(int test)
{
  int limit = 8192;
  int compare = 1;
  //  post("testing what we thing is an int:%d",test);
  do {
    if(test == compare){
      //      post("good power of 2 found!");
      return 1;
    } 
    compare *= 2;
  } while (compare <= limit);
  
  return 0;
}

