(declaim (optimize (speed 0) (safety 3) (debug 3) (size 0)))
(defpackage :cl-gui-base (:use :cl)
	    (:export :string-id :get-table-count :get-table-data
		     :set-property :get-property :make-window :draw-windows))
(in-package :cl-gui-base)


(defpackage :cl-gui (:use :cl :cl-gui-base)
	    (:export :setprop :getprop :new-gui-object  :getid))

(in-package :cl-gui)
(defstruct gui-object
  (id 0 :type fixnum))

(defun new-gui-object(name)
  (make-gui-object :id (if (integerp name) name (string-id name))))

(defvar properties-lookup (make-hash-table :test #'eq))
(defvar propcnt 0)

(defun find-prop (name)
  (let ((cnt (get-table-count)))
    (unless (eq cnt propcnt)
      (setf propcnt cnt)
      (dotimes (i cnt)
	(let ((table-data (get-table-data i)))
	  (setf (gethash (car table-data) properties-lookup)
		i)))))
  (gethash name properties-lookup))

(defun setprop (guiobj name value)
  (let ((id (find-prop name)))
    (when id
      (set-property id (gui-object-id guiobj) value))))

(defun getprop (guiobj name)
  (let ((id (find-prop name)))
    (when id
      (get-property id (gui-object-id guiobj)))))
  
(defun getid (guiobj)
  (gui-object-id guiobj))
