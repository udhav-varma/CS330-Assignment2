#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
#include<tracer.h>


///////////////////////////////////////////////////////////////////////////
//// 		Start of Trace buffer functionality 		      /////
///////////////////////////////////////////////////////////////////////////

int is_valid_mem_range(unsigned long buff, u32 count, int access_bit) 
{
	return 0;
}



long trace_buffer_close(struct file *filep)
{
	return 0;	
}



int trace_buffer_read(struct file *filep, char *buff, u32 count)
{
	return 0;
}


int trace_buffer_write(struct file *filep, char *buff, u32 count)
{
    	return 0;
}

int sys_create_trace_buffer(struct exec_context *current, int mode)
{
	int free_file_number = 0;
	for(free_file_number = 0; free_file_number < MAX_OPEN_FILES; free_file_number++){
		if(current->files[free_file_number] == NULL) break;
	}
	if(free_file_number == MAX_OPEN_FILES) return -EINVAL;
	current->files[free_file_number] = (struct file *) os_alloc(sizeof(struct file));
	if(current->files[free_file_number] == NULL) return -ENOMEM;
	struct file* fileobj = current->files[free_file_number];
	fileobj->trace_buffer = (struct trace_buffer_info *) os_alloc(sizeof(struct trace_buffer_info));
	if(fileobj->trace_buffer == NULL) return -ENOMEM;
	// TODO - Initialise trace_buffer_info object, based on implementation
	fileobj->fops = (struct fileops *) os_alloc(sizeof(struct fileops));
	if(fileobj->fops == NULL) return -ENOMEM;
	fileobj->fops->read = trace_buffer_read;
	fileobj->fops->write = trace_buffer_write;
	fileobj->fops->close = trace_buffer_close;
	return free_file_number;
}

///////////////////////////////////////////////////////////////////////////
//// 		Start of strace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////


int perform_tracing(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4)
{
    return 0;
}


int sys_strace(struct exec_context *current, int syscall_num, int action)
{
	return 0;
}

int sys_read_strace(struct file *filep, char *buff, u64 count)
{
	return 0;
}

int sys_start_strace(struct exec_context *current, int fd, int tracing_mode)
{
	return 0;
}

int sys_end_strace(struct exec_context *current)
{
	return 0;
}



///////////////////////////////////////////////////////////////////////////
//// 		Start of ftrace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////


long do_ftrace(struct exec_context *ctx, unsigned long faddr, long action, long nargs, int fd_trace_buffer)
{
    return 0;
}

//Fault handler
long handle_ftrace_fault(struct user_regs *regs)
{
        return 0;
}


int sys_read_ftrace(struct file *filep, char *buff, u64 count)
{
    return 0;
}


