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
	(id (or (plist-get body :id) "testwin")));(assert false))))
    (let ((obj (cl-gui:new-gui-object id)))
      ;(set-property obj :width width)
      (apply #'list obj :title title :height height :width width (plist-collect body)))))

(defun button (&rest body)
  (let ((click (plist-get body :click))
	(content (or (plist-get body :content) "button")))
    (list :click click :content content)))
(export 'button)
;(format t ":: ~a ~%" (window  :title "asd" :height 100 :width 100 :test 555))
(format t "~a ~%" (getf '(:a 1 :b 2 3 5 :c "asd") :c))
(format t "~a ~%" (window 1 :width 100))

(format t "~a ~%" (window :title "My win" :width 50 :height 50
	(button :click (lambda () (print "Clicked!"))
		:content "Click me!")
		(button :click (lambda () (print "Clicked!"))
		:content "Click me too!")
		))

; Testing gui ids
(print (cl-gui-base:string-id "555"))
(print (cl-gui-base:string-id "556"))
(print (cl-gui-base:string-id "555"))

(defvar win (window :id "win1" :title "My Window" :width 200 :height 200))
(print win);
(print (cl-gui-base:get-table-count))
(defvar tab1 (cl-gui-base:get-table-data 1))
(defvar set-value (cl-gui-base:get-property 1 (cl-gui-base:string-id "555")));
(defvar set-color (cl-gui-base:get-property 2 (cl-gui-base:string-id "555"))); 
(defmacro comment (&rest body) ())
(comment (let ((i 0))
  (cl-gui-base:set-property 0 (cl-gui-base:string-id "555") (list i 2.0))
  (cl-gui-base:set-property 2 (cl-gui-base:string-id "555") (list i 1.0 2.0 4.0))
  (setf set-value (cl-gui-base:get-property 1 (cl-gui-base:string-id "555")));
  (setf set-color (cl-gui-base:get-property 2 (cl-gui-base:string-id "555")));
  ))
  (print tab1)
(print (eq (car tab1) 'SIZE))
(print (list "Got value: " set-value set-color))
;(clgui:setprop 'size 
;(print (list "Window ID: " (gui-object-id (car win))))

;(print (list "Prev size:" (cl-gui-base:get-property 1 (gui-object-id (car win)))))

;(cl-gui-base:set-property 1 (gui-object-id (car win)) (list 200 200))
;(cl-gui-base:set-property 3 (gui-object-id (car win)) (list 50 50))
;(cl-gui-base:set-property 2 (gui-object-id (car win)) (list 0.0 0.0 1.0 1.0))
;(print (cl-gui-base:get-property 1 (gui-object-id (car win))))  
;(cl-gui-base:make-window (gui-object-id (car win)))

;(print *features*)


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
