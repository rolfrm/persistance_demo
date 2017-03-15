;(load "/usr/share/common-lisp/source/slime/swank-loader.lisp")
(declaim (optimize (speed 0) (safety 3) (debug 3) (size 0)))

(defpackage :test (:use :cl) (:export :button :cos2 :test :test-cfun :clgui-object))
(in-package :test)
(defmacro comment (&rest body) ())
(format t "test loaded~%")

(defun plist-get (list sym)
  (when list
    (if (eq (car list) sym)
	(cadr list)
	(plist-get (cdr list) sym))))

(defun plist-collect (list)
  (when list
    (if (not (keywordp (car list)))
	(cons (car list) (plist-collect (cdr list)))
	(plist-collect (cddr list)))))

(defun load-ids (form)
  (let ((id (or (plist-get form :id) (error "root form must have an id"))))
    (labels ((load-form (form id)
	     (let ((collect (plist-collect (cdr form)))
		   (cnt 0))
	       (print (list "form id: " id))
	       (cons :obj (cons (cl-gui:new-gui-object id) form))

	       (loop while collect do
		    (print (car collect))
		    (load-form (car collect) (format nil "~a.~a" id cnt))
		    (setf collect (cdr collect))
		    (incf cnt))
	       )))
      (load-form form id))))
    
    
    
    


(defstruct (reference
	     (:print-function
	      (lambda (struct stream depth)
		(format stream "<ref to '~a'(~a)>" (reference-name struct) (funcall (reference-get struct)))))
	     )
	       
  (set ())
  (get ())
  (name ())
  )

(defmacro ref(x)
  `(make-reference
    :set (lambda (value) (setf ,x value))
    :get (lambda () ,x)
    :name ',x))

(defun deref (ref)
  (funcall (reference-get ref)))

(defun (setf deref) (value ref)
  (funcall (reference-set ref) value))

(defun window (&rest body )
  (cons 'window body))
(defun textblock(&rest body)
  (cons 'textblock body))
(defun button (&rest body)
  (cons 'button body))
(defun grid (&rest body)
  (cons 'grid body))


(let ((width 100) (height 100))
  (defparameter win
    (window
     :id "win1" :title "My Window" :width (ref width) :height (ref height)
     (grid
      (textblock :grid-row 0 :text "Just click the button")
      (button
       :grid-row 1 :content "Click me!"
       :click (lambda () (incf width 10) (incf height 100))
       )
      )
     )
    )
  
  (print win);
  (load-ids win)
  )

(comment 





(import :ffi)
(ffi:clines "
#include <GL/glew.h>
#include <iron/full.h>
#include \"persist.h\"
#include \"sortable.h\"
#include \"persist_oop.h\"
#include \"shader_utils.h\"
#include \"game.h\"

#include <GLFW/glfw3.h>

#include \"gui.h\"
#include \"gui_test.h\"
#include \"index_table.h\"
#include \"simple_graphics.h\"
#include \"game_board.h\"
")

(defun create-table (name elem-size)
  (ffi:c-inline (name elem-size) (:c-string-pointer :unsigned-int) :pointer-void "index_table_create(#0,#1)" :one-liner t)
  )

(defun test()
					;(create-table  +null-cstring-pointer+ 4)
  (print ffi:+null-cstring-pointer+)
  (print "ok.."));

(in-package :cl-user)
(print (test:button))

)
