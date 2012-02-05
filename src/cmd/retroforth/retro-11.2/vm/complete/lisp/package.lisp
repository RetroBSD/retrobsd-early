; Copyright (c) 2011, Oleksandr Kozachuk

(in-package :cl-user)

(defpackage :retro
  (:use :cl
        #+ecl  :cffi
        #+sbcl :sb-alien)
  (:export :ngaro
           :add-input-file

           :*image-file*
           :*image-size*
           :*addresses-depth*
           :*stack-depth*
           :*files-depth*))
