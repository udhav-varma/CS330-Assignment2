#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
#include<tracer.h>
///////////////////////////////////////////////////////////////////////////
//// 		Start of Trace buffer functionality 		      /////
///////////////////////////////////////////////////////////////////////////
/**
 * Returns 1 if valid mem range, 0 if not a valid mem range, -1 for error
*/
int is_valid_mem_range(unsigned long buff, u32 count, int access_bit) 
{
	if(count < 0) return -1;
	struct exec_context * current_pcb = get_current_ctx();
	for(int seg = 0; seg < MAX_MM_SEGS; seg++){
		struct mm_segment* current_seg = &(current_pcb->mms[seg]);
		if((current_seg->access_flags&access_bit) == (access_bit)){
			unsigned long start = current_seg->start;
			unsigned long end = ((seg == MM_SEG_STACK)?current_seg->end:current_seg->next_free);
			if(buff >= start && (buff + count <= end)) return 1;
		}
	}
	struct vm_area * current_vm = current_pcb->vm_area;
	while(current_vm != NULL){
		if((current_vm->access_flags&access_bit) == (access_bit)){
			if(buff >= (current_vm->vm_start) && (buff + count <= (current_vm->vm_end))) return 1;
		}
		current_vm = current_vm->vm_next;
	}
	return 0;
}



long trace_buffer_close(struct file *filep)
{
	os_page_free(USER_REG, filep->trace_buffer->trace_buffer);
	os_free(filep->trace_buffer, sizeof(struct trace_buffer_info));
	os_free(filep->fops, sizeof(struct fileops));
	os_free(filep, sizeof(struct file));
	return 0;	
}



int trace_buffer_read(struct file *filep, char *buff, u32 count)
{
	if(filep == NULL || buff == NULL) return -EINVAL;
	int valid_flag = is_valid_mem_range((unsigned long) buff, count, 2);
	if(valid_flag == -1) return -EINVAL;
	else if(valid_flag == 0) return -EBADMEM;
	struct trace_buffer_info * trace_buff = filep->trace_buffer;
	if(trace_buff == NULL) return -EINVAL;
	if(trace_buff->occupy == 0) return 0;
	int size = trace_buff->occupy;
	count = ((size < count)?size:count);
	for(int i = 0; i < count; i++){
		buff[i] = trace_buff->trace_buffer[(trace_buff->readPos + i)%TRACE_BUFFER_MAX_SIZE];
	}
	trace_buff->readPos = (trace_buff->readPos + count)%TRACE_BUFFER_MAX_SIZE;
	trace_buff->occupy -= count;
	return count;
}


int trace_buffer_write(struct file *filep, char *buff, u32 count)
{
	if(filep == NULL) return -EINVAL;
	if(buff == NULL) return -EINVAL;
	int valid_flag = is_valid_mem_range((unsigned long) buff, count, 1);
	if(valid_flag == -1) return -EINVAL;
	else if(valid_flag == 0) return -EBADMEM;
	struct trace_buffer_info * trace_buff = filep->trace_buffer;
	if(trace_buff == NULL) return -EINVAL;
	if(trace_buff->occupy == TRACE_BUFFER_MAX_SIZE) return 0;
	int remainSize = TRACE_BUFFER_MAX_SIZE - (trace_buff->occupy);
	count = ((remainSize < count)?remainSize:count);
	for(int i = 0; i < count; i++){
		trace_buff->trace_buffer[(trace_buff->writePos + i)%TRACE_BUFFER_MAX_SIZE] = buff[i];
	}
	trace_buff->writePos = ((trace_buff->writePos + count)%TRACE_BUFFER_MAX_SIZE);
	trace_buff->occupy += count;
	return count;
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
	fileobj->type = TRACE_BUFFER;
	fileobj->mode = mode;
	fileobj->inode = NULL;
	fileobj->offp = 0;
	fileobj->ref_count = 1;
	fileobj->trace_buffer = (struct trace_buffer_info *) os_alloc(sizeof(struct trace_buffer_info));
	fileobj->trace_buffer->trace_buffer = (char *) os_page_alloc(USER_REG);
	if(fileobj->trace_buffer->trace_buffer == NULL) return -ENOMEM;
	if(fileobj->trace_buffer == NULL) return -ENOMEM;
	fileobj->trace_buffer->occupy = 0;
	fileobj->trace_buffer->readPos = 0;
	fileobj->trace_buffer->writePos = 0;
	// TODO - Initialise trace_buffer_info object, based on implementation
	fileobj->fops = (struct fileops *) os_alloc(sizeof(struct fileops));
	if(fileobj->fops == NULL) return -ENOMEM;
	fileobj->fops->read = &trace_buffer_read;
	fileobj->fops->write = &trace_buffer_write;
	fileobj->fops->close = &trace_buffer_close;
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


