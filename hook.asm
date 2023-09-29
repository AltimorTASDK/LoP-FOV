_TEXT SEGMENT

EXTERN set_camera_component_fov: PROC

PUBLIC hook_ALCameraManager_SetupDefaultCamera_set_fov
hook_ALCameraManager_SetupDefaultCamera_set_fov PROC
        mov     rcx, rdi
        mov     rdx, r9
        jmp     set_camera_component_fov
hook_ALCameraManager_SetupDefaultCamera_set_fov ENDP

EXTERN set_spring_arm_length: PROC

PUBLIC hook_ALCameraManager_UpdateCamera_set_dist
hook_ALCameraManager_UpdateCamera_set_dist PROC
        mov     rcx, r9
        movaps  xmm1, xmm0
        ; Must preserve r9
        jmp     set_spring_arm_length
hook_ALCameraManager_UpdateCamera_set_dist ENDP

EXTERN set_spring_arm_lag: PROC

PUBLIC hook_ALCameraManager_UpdateCamera_set_lag
hook_ALCameraManager_UpdateCamera_set_lag PROC
        mov     rcx, r9
        movaps  xmm1, xmm0
        ; Must preserve r9
        sub     rsp, 20h
        call    set_spring_arm_lag
        add     rsp, 20h
        ; Restore rcx
        lea     rcx, [rbx+29F0h]
        ret
hook_ALCameraManager_UpdateCamera_set_lag ENDP

EXTERN distance_multiplier: DWORD

PUBLIC hook_FLRotationAccordingToMovement_UpdateRotation_scale
hook_FLRotationAccordingToMovement_UpdateRotation_scale PROC
        movss   dword ptr [rsp+8+48h], xmm0
        movss   xmm0, [distance_multiplier]
        mulss   xmm6, xmm0
        addss   xmm6, dword ptr [rsp+8+34h]
        ret
hook_FLRotationAccordingToMovement_UpdateRotation_scale ENDP

_TEXT ENDS

END
