(require :asdf)
(require :retro)

(defun parse-arguments (args)
  (cond ((string-equal (car args) "--with")
         (retro:add-input-file (cadr args))
         (parse-arguments (cddr args)))
        ((string-equal (car args) "--image")
         (setf retro:*image-file* (cadr args))
         (parse-arguments (cddr args)))
        ((null args) nil)
        (t (format *error-output* "ERROR: Unknown option ~S~%" (car args))
           (format *error-output* "~%Usage: ~S [--with <input file>] [--image <image file>]~%"
                   (car sb-ext:*posix-argv*))
           (sb-ext:quit))))

(defun main ()
  (parse-arguments (cdr sb-ext:*posix-argv*))
  (handler-case (retro:ngaro)
    (error (msg) (format *error-output* "~%ERROR: ~A~%" msg)))
  (fresh-line))

(save-lisp-and-die "../../../retro"
                 :purify t
                 :executable t
                 :toplevel #'main)
