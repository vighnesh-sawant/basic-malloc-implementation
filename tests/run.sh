echo "RUNNING $1"
env LD_PRELOAD=./malloc.so $1; 
if [ $? -ne 0 ]; then 
  echo "TEST FAILED" 
  exit 1
fi; 

