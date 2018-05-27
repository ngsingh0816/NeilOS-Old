/*
 * Rather than create a case for each number of arguments, we simplify
 * and use one macro for up to three arguments; the system calls should
 * ignore the other registers, and they're caller-saved anyway.
 */
#define DO_CALL(name,number)   \
.globl name                   ;\
name:                         ;\
pushl %ebx                    ;\
pushl %ecx                    ;\
pushl %edi                    ;\
pushl %esi                    ;\
pushl %edi                      ;\
pushl %ebp                      ;\
movl $number, %eax            ;\
movl 28(%esp), %ebx           ;\
movl 32(%esp), %ecx           ;\
movl 36(%esp), %edx           ;\
movl 40(%esp), %esi           ;\
movl 44(%esp), %edi              ;\
movl 48(%esp), %ebp              ;\
int    $0x80                     ;\
popl %ebp                      ;\
popl %edi                      ;\
popl %esi                     ;\
popl %edi                     ;\
popl %ecx                     ;\
popl %ebx                     ;\
ret

DO_CALL(sys_graphics_fb_map, 49)
DO_CALL(sys_graphics_fb_unmap, 50)
DO_CALL(sys_graphics_info_get, 51)
DO_CALL(sys_graphics_info_set, 52)
DO_CALL(sys_graphics_update_rect, 53)
DO_CALL(sys_graphics_cursor_image_set, 54)
DO_CALL(sys_graphics_fence_create, 55)
DO_CALL(sys_graphics_fence_sync, 56)
DO_CALL(sys_graphics_fence_passed, 57)
DO_CALL(sys_graphics_present, 58)
DO_CALL(sys_graphics_present_readback, 59)
DO_CALL(sys_graphics_surface_create, 60)
DO_CALL(sys_graphics_surface_dma, 61)
DO_CALL(sys_graphics_surface_copy, 62)
DO_CALL(sys_graphics_surface_stretch_copy, 63)
DO_CALL(sys_graphics_surface_reformat, 64)
DO_CALL(sys_graphics_surface_destroy, 65)
DO_CALL(sys_graphics_context_create, 66)
DO_CALL(sys_graphics_context_destroy, 67)
DO_CALL(sys_graphics_clear, 68)
DO_CALL(sys_graphics_draw, 69)
DO_CALL(sys_graphics_state_render_target, 70)
DO_CALL(sys_graphics_state_z_range, 71)
DO_CALL(sys_graphics_state_viewport, 72)
DO_CALL(sys_graphics_state_scissor_rect, 73)
DO_CALL(sys_graphics_state_clip_plane, 74)
DO_CALL(sys_graphics_state_texture_state, 75)
DO_CALL(sys_graphics_state_render_state, 76)
DO_CALL(sys_graphics_transform_set, 77)
DO_CALL(sys_graphics_light_material, 78)
DO_CALL(sys_graphics_light_data, 79)
DO_CALL(sys_graphics_light_enabled, 80)
DO_CALL(sys_graphics_shader_create, 81)
DO_CALL(sys_graphics_shader_const, 82)
DO_CALL(sys_graphics_shader_set_active, 83)
DO_CALL(sys_graphics_shader_destroy, 84)
