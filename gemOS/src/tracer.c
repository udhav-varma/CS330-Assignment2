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
	if(filep == NULL) return -EINVAL;
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
	if(mode < 1 || mode > 3) return -EINVAL;
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

int trace_buffer_read_unsafe(struct file *filep, char *buff, u32 count)
{
	if(filep == NULL || buff == NULL) return -EINVAL;
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

int trace_buffer_write_unsafe(struct file *filep, char *buff, u32 count)
{
	if(filep == NULL) return -EINVAL;
	if(buff == NULL) return -EINVAL;
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

int perform_tracing(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4)
{
	struct exec_context * curr_ctx = get_current_ctx();
	struct strace_head * head = curr_ctx->st_md_base;
	// if(head == NULL) printk("Headnull %d\n", syscall_num);
	// else if(head->is_traced == 0) ("head not traced %d\n", syscall_num);
	if(head == NULL || head->is_traced == 0) return 0;
	// printk("here %d\n", (int) syscall_num);
	int to_be_tracked = 0;
	if(head->tracing_mode == FULL_TRACING) to_be_tracked = 1;
	else{
		struct strace_info* beg = head->next;
		while(beg != NULL){
			if(beg->syscall_num == syscall_num){
				to_be_tracked = 1;
				break;
			}
			beg = beg->next;
		}
	}
	if(to_be_tracked != 1) return 0;
	// printk(" to be tracked\n");
	int num_arg = 4;
	switch (syscall_num) {
		case SYSCALL_EXIT:
			num_arg = 1;
			break;
		case SYSCALL_GETPID:
			num_arg = 0;
			break;
		case SYSCALL_EXPAND:
			num_arg = 2;
			break;
		case SYSCALL_SHRINK:
			num_arg = 1;
			break;
		case SYSCALL_ALARM:
			num_arg = 1;
			break;
		case SYSCALL_SLEEP:
			num_arg = 1;
			break;
		case SYSCALL_SIGNAL:
			num_arg = 2;
			break;
		case SYSCALL_CLONE:
			num_arg = 2;
			break;
		case SYSCALL_FORK:
			num_arg = 0;
			break;
		case SYSCALL_STATS:
			num_arg = 0;
			break;
		case SYSCALL_CONFIGURE:
			num_arg = 1;
			break;
		case SYSCALL_PHYS_INFO:
			num_arg = 0;
			break;
		case SYSCALL_DUMP_PTT:
			num_arg = 1;
			break;
		case SYSCALL_CFORK:
			num_arg = 0;
			break;
		case SYSCALL_MMAP:
			num_arg = 4;
			break;
		case SYSCALL_MUNMAP:
			num_arg = 2;
			break;
		case SYSCALL_MPROTECT:
			num_arg = 3;
			break;
		case SYSCALL_PMAP:
			num_arg = 1;
			break;
		case SYSCALL_VFORK:
			num_arg = 0;
			break;
		case SYSCALL_GET_USER_P:
			num_arg = 0;
			break;
		case SYSCALL_GET_COW_F:
			num_arg = 0;
			break;
		case SYSCALL_OPEN:
			num_arg = (param2&O_CREAT == 0?2:3);
			break;
		case SYSCALL_READ:
			num_arg = 3;
			break;
		case SYSCALL_WRITE:
			num_arg = 3;
			break;
		case SYSCALL_DUP:
			num_arg = 1;
			break;
		case SYSCALL_DUP2:
			num_arg = 2;
			break;
		case SYSCALL_CLOSE:
			num_arg = 1;
			break;
		case SYSCALL_LSEEK:
			num_arg = 3;
			break;
		case SYSCALL_FTRACE:
			num_arg = 4;
			break;
		case SYSCALL_TRACE_BUFFER:
			num_arg = 1;
			break;
		case SYSCALL_START_STRACE:
			num_arg = 2;
			break;
		case SYSCALL_END_STRACE:
			return 0;
		case SYSCALL_READ_STRACE:
			num_arg = 3;
			break;
		case SYSCALL_STRACE:
			num_arg = 2;
			break;
		case SYSCALL_READ_FTRACE:
			num_arg = 3;
			break;
		case SYSCALL_GETPPID:
			num_arg = 0;
			break;
	}
	
	struct file* trace_buffer = curr_ctx->files[head->strace_fd];
	
	u64 * write_data = (u64 *) os_alloc((num_arg + 2)*sizeof(u64));
	write_data[0] = num_arg;
	write_data[1] = syscall_num;
	u64 args[4] = {param1, param2, param3, param4};
	for(int i = 2; i < num_arg + 2; i++){
		write_data[i] = args[i - 2];
	}
	int written_bytes = trace_buffer_write_unsafe(trace_buffer, (char *) write_data, (num_arg + 2)*sizeof(u64));
	// printk("syscall - %d, num_arg - %d, written_bytes - %d\n", syscall_num, num_arg, written_bytes);
    return 0;
}


int sys_strace(struct exec_context *current, int syscall_num, int action)
{
	if(current == NULL) return -EINVAL;
	struct strace_head * head = current->st_md_base;
	if(head == NULL){
		if(action == REMOVE_STRACE) return -EINVAL;
		head = (struct strace_head *) os_alloc(sizeof(struct strace_head));
		head->count = 1;
		struct strace_info * thiscall = os_alloc(sizeof(struct strace_info));
		thiscall->syscall_num = syscall_num;
		thiscall->next = NULL;
		head->next = thiscall;
		head->last = thiscall;
		current->st_md_base = head;
		return 0;
	}
	if(action == ADD_STRACE){
		// printk("Adding %d\n", syscall_num);
		if(head->next == NULL){
			struct strace_info * thiscall = os_alloc(sizeof(struct strace_info));
			thiscall->syscall_num = syscall_num;
			thiscall->next = NULL;
			head->next = thiscall;
			head->last = thiscall;
			head->count++;
		}
		else{
			struct strace_info* pos = head->next;
			struct strace_info * ppos = pos;
			while(pos != NULL && pos->syscall_num != syscall_num){
				ppos = pos;
				pos = pos->next;
			}
			if(pos == NULL){
				if(head->count == STRACE_MAX){
					return -EINVAL;
				}
				struct strace_info * newnode = os_alloc(sizeof(struct strace_info));
				newnode->next = NULL;
				newnode->syscall_num = syscall_num;
				ppos->next = newnode;
				head->last = newnode;
				head->count++;
			}
		}
	}
	else if(action == REMOVE_STRACE){
		struct strace_info* pos = head->next;
		struct strace_info * ppos = pos;
		while(pos != NULL && pos->syscall_num != syscall_num){
			ppos = pos;
			pos = pos->next;
		}
		if(pos == NULL) return -EINVAL;
		head->count--;
		if(pos == head->next){
			head->next = head->next->next;
			if(head->next == NULL){
				head->last = NULL;
			}
			os_free(pos, sizeof(struct strace_info));
		}
		else{
			ppos->next = pos->next;
			if(pos->next == NULL) head->last = ppos;
			os_free(pos, sizeof(struct strace_info));	
		}
	}
	else return -EINVAL;
	return 0;
}

int sys_read_strace(struct file *filep, char *buff, u64 count)
{
	if(filep == NULL) return -EINVAL;
	if(buff == NULL) return -EINVAL;
	if(count < 0) return -EINVAL;
	int buff_pos = 0;
	while(count > 0){
		u64 * ptr = os_alloc(sizeof(u64));
		int r = trace_buffer_read_unsafe(filep, (char *) ptr, sizeof(u64));
		if(r == 0) return buff_pos;
		if(r != sizeof(u64)){
			// printk("Error1 - read %d bytes\n", r);
			return -EINVAL;
		}
		u64 num_arg = *ptr;
		for(int i = 0; i <= num_arg; i++){
			int r = trace_buffer_read_unsafe(filep, buff + buff_pos, sizeof(u64));
			if(r != sizeof(u64)){
				// printk("Error2 - read %d bytes\n", r);
				return -EINVAL;
			}
			buff_pos += sizeof(u64);
		}
		os_free(ptr, sizeof(u64));
		count -= 1;
	}
	return buff_pos;
}

int sys_start_strace(struct exec_context *current, int fd, int tracing_mode)
{
	if(current == NULL) return -EINVAL;
	struct strace_head * head = current->st_md_base;
	int inititally_allocated = 1;
	if(head == NULL){
		head = (struct strace_head *) os_alloc(sizeof(struct strace_head));
		current->st_md_base = head;
		inititally_allocated = 0;
	}
	if(head == NULL) return -EINVAL;
	if(inititally_allocated){
		head->is_traced = 1;
		head->strace_fd = fd;
		head->tracing_mode = tracing_mode;
	}
	else{
		head->count = 0;
		head->is_traced = 1;
		head->strace_fd = fd;
		head->tracing_mode = tracing_mode;
		head->next = NULL;
		head->last = NULL;
	}
	return 0;
}

int sys_end_strace(struct exec_context *current)
{
	if(current == NULL) return -EINVAL;
	struct strace_head * head = current->st_md_base;
	if(head == NULL) return -EINVAL;
	struct strace_info * pos = head->next;
	while(pos != NULL){
		struct strace_info * npos = pos->next;
		os_free(pos, sizeof(struct strace_info));
		pos = npos;
	}
	os_free(head, sizeof(struct strace_head));
	current->st_md_base = NULL;
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


