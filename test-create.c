#include <stdio.h>
#include <stdlib.h>
#include "unixv6fs.h"
#include "bmblock.h"
#include "mount.h"
#include "error.h"
#include "filev6.h"

int main(){
	struct unix_filesystem *u=NULL;
	mountv6("../disks/simple.uv6",u);
	struct inode i;
	struct filev6* fv6=calloc(1,sizeof(struct filev6));
	fv6->u = u;
	fv6->i_number=3;
	fv6->i_node=i;
	fv6->offset=0;
	filev6_create(u,0,fv6);
	free(fv6);
	return 0;
}
