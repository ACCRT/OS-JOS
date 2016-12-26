// User-level IPC library routines

#include <inc/lib.h>

// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'from_env_store' is nonnull, then store the IPC sender's envid in
//	*from_env_store.
// If 'perm_store' is nonnull, then store the IPC sender's page permission
//	in *perm_store (this is nonzero iff a page was successfully
//	transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
// Otherwise, return the value sent by the sender
//
// Hint:
//   Use 'thisenv' to discover the value and who sent it.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value, since that's
//   a perfectly valid place to map a page.)
int32_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	// LAB 4: Your code here.
	//panic("ipc_recv not implemented");
	//return 0;
	
	// for sfork challenge, we should repalce thisenv by envs+ENVX(sys_getenvid())
	// because thisenv is a global variable defined in lib/libmain.c
	// which means that if we use thisenv in sfork, thisenv is write shared
	// the parent's thisenv will be changed by child and point to the child's env
	
	if (!pg) pg = (void*)UTOP;
	int r = sys_ipc_recv(pg);
	if (r >= 0) {
		if(perm_store != NULL)
			//*perm_store = thisenv->env_ipc_perm;
			*perm_store = envs[ENVX(sys_getenvid())].env_ipc_perm;
		if(from_env_store != NULL)
			//*from_env_store = thisenv->env_ipc_from;
			*from_env_store = envs[ENVX(sys_getenvid())].env_ipc_from;
		//return thisenv->env_ipc_value;
		return envs[ENVX(sys_getenvid())].env_ipc_value;
	}
	if(perm_store != NULL)
		*perm_store = 0;
	if(from_env_store != NULL)
		*from_env_store = 0;
	return r;
}

// Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
//
// Hint:
//   Use sys_yield() to be CPU-friendly.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
void
ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
{
	// LAB 4: Your code here.
	//panic("ipc_send not implemented");
	if(!pg) pg = (void*)UTOP; 
	int r;
	while((r = sys_ipc_try_send(to_env,val,pg,perm)) != 0)
	{
		if(r != -E_IPC_NOT_RECV )
			//panic ("[lib/ipc.c ipc_send]: sys try send failed : %e", r);
			break;
	}
	sys_yield();
}

// Find the first environment of the given type.  We'll use this to
// find special environments.
// Returns 0 if no such environment exists.
envid_t
ipc_find_env(enum EnvType type)
{
	int i;
	for (i = 0; i < NENV; i++)
		if (envs[i].env_type == type)
			return envs[i].env_id;
	return 0;
}