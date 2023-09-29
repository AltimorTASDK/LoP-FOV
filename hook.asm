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

EXTERN rotate_multiplier: REAL4

PUBLIC hook_FLRotationAccordingToMovement_UpdateRotation_scale
hook_FLRotationAccordingToMovement_UpdateRotation_scale PROC
        ; Scale rotation amount before it gets processed
        ucomiss xmm2, xmm0
        shufps  xmm6, xmm6, 55h
        mulss   xmm6, rotate_multiplier
        movaps  xmm3, xmm6
        andps   xmm3, xmm4
        ret
hook_FLRotationAccordingToMovement_UpdateRotation_scale ENDP

_TEXT ENDS

END
