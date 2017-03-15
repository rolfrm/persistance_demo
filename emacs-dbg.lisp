(defun start-dbg()

  (get-buffer-create "*gud*")
  (get-buffer-create "slime")
  (interactive)
  (with-current-buffer "*gud*"
    (gud-gdb "./run.exe")
    
    )
  (sit-for 0.5)   
  (with-current-buffer "slime"
    (slime-connect "127.0.0.1" 4005 :nowait nil)
    (sit-for 0.5)
    ;(slime-load-file "test.lisp")
    )  
  )

(start-dbg)

