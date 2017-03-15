;(load "/usr/share/common-lisp/source/slime/swank-loader.lisp")
(declaim (optimize (speed 0) (safety 3) (debug 3) (size 0)))

(defpackage :test (:use :cl) (:export :button :cos2 :test :test-cfun :clgui-object))
(in-package :test)
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

(defun window (&rest body )
  (let ((width (or (plist-get body :width) 50))
	(height (or (plist-get body :height) 50))
	(title (or (plist-get body :title) "Name me"))
	(id (or (plist-get body :id) "testwin"))
	(collect (plist-collect body)));(assert false))))
    
    (flet ((load (id)
	     (let ((obj (cl-gui:new-gui-object id)))
	       (cl-gui:setprop obj :size  '(150 150))
	       (cl-gui:setprop obj :title  "My test window") 
	       (cl-gui:setprop obj :color-alpha '(1 0.8 0.2 1))
	       (let ((collectiter collect))
		 (loop while collectiter do
		      (print (list "iter: " (caar collectiter)))
		      (progn
			(when (functionp (caar collectiter))
			  (setf (caar collectiter) (funcall (caar collectiter) (+ (cl-gui:getid obj) 10000))))
			(setf collectiter (cdr collectiter)))))
	       obj
	       )))
      
      (apply #'list (if id (load id) #'load) :title title :height height :width width collect))))

(defun textblock(&rest body)
  (let ((text (plist-get body :text))
	(id (plist-get body :id)))
    (flet ((load (id)
	     (let ((obj (cl-gui:new-gui-object id)))
	       (cl-gui:setprop obj :text  "My test window")
	       (let ((collect (plist-collect body)))
	       (apply #'list (if id (load id) #'load)
		      collect))))))))

(defun button (&rest body)
  (let ((click (plist-get body :click))
	(content (or (plist-get body :content) "button"))
	(id (plist-get body :id)))
    (flet ((load (id)
	     (let ((obj (cl-gui:new-gui-object id)))
	       (setf content
		     (if (stringp content)
			 (setf content (textblock :text content))
			 content))
	       obj)))

      (apply #'list (if id (load id) #'load)
	     :click click
	     :content content
	     (plist-collect body)))))

(defparameter win (window :id "win1" :title "My Window" :width 200 :height
		    200 (button :content "Click me!")))
(print win);

(defstruct reference
  (set ())
  (get ()))

(defmacro ref(x)
  `(make-reference
    :set (lambda (value) (setf ,x value))
    :get (lambda () ,x)))

(defun deref (ref)
  (funcall (reference-get ref)))

(defun (setf deref) (value ref)
  (funcall (reference-set ref) value))

(let ((y-ref
       (let ((y 10))
	 (ref y))))
  (setf (deref y-ref) 15)
  (incf (deref y-ref))
  (print (deref y-ref)))
    


(defmacro comment (&rest body) ())

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
