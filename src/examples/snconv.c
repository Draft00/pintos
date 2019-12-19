#include <stdio.h>
#include <syscall.h>
int pow(int a, int b)
{
int z=1;
 for(int i=1;i<=b;i++)
        z=z*a;
return z;
}
int 
main(int argc UNUSED, const char* argv[])
{
int i=1;
int a[3];
while(argv[i])
{
a[i-1]=atoi(argv[i]);
i++;
}
//for(int d=0;d<3;d++)
//printf("%d \n", a[d]);
int q=a[0];
int k=0;
while (q>0)
{
  k++;
  q=q/10;
}
int w=k;
int sum=0;
int b=0;
if (a[1]==2)
{
     while(a[0]>0)
{
     
     b=(a[0]%pow(10,w-1))*pow(2,k-w);
if (w==1)
{
    b=a[0]*pow(2,k-w);
}
     w--;
     a[0]=a[0]/10;
     sum=sum+b;
}
}
printf("%i\n",sum);
return EXIT_SUCCESS;
}
