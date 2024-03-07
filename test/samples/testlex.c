// program de testare a analizorului lexical, v1.1

int main()
{
	int i;
	i=0;
	while(i<10){
		if(i/2==1)puti(i);
		i=i+1;
		}
	if(4.9==49e-1&&0.49E1==2.45*2.0)puts("yes");
	putc('#');
	puts("\n");	// pentru \n

	struct S
	{
		int x;
		double y;
		char z;
		char str[16];
	};

	S s;
	s.x = 134541;
	s.y = 1.03E-7;
	strcpy(s.str, "Hello, World!\n");

	return 0;
}
