; Copyright (c) 2011, Oleksandr Kozachuk

(in-package :retro)

; speed 3 and safety 0 is required to pass all tests
; (the random function produces overruns in the numbers)
(declaim (optimize (speed 3) (space 0) (safety 0) (debug 0)))

(defparameter *image-file* "retroImage")
(defparameter *image-size*      1000000)
(defparameter *addresses-depth*    1024)
(defparameter *stack-depth*         128)
(defparameter *ports-depth*          12)
(defparameter *files-depth*        1024)

(defconstant +cell-size+ 32)

(deftype cell () `(signed-byte ,+cell-size+))
#+(or big-endian ecl)
(setf *image-file* (concatenate 'string *image-file* "BE"))

(defun create-data (size)
  (declare (type fixnum size))
  (make-array size :element-type 'cell :initial-element 0))

(declaim (type cell
               *image-size*
               *addresses-depth*
               *stack-depth*
               *ports-depth*
               *files-depth*))

(defparameter *image*     (create-data *image-size*))
(defparameter *addresses* (create-data *addresses-depth*))
(defparameter *data*      (create-data *stack-depth*))
(defparameter *ports*     (create-data *ports-depth*))
(defparameter *files*     (make-array *files-depth*
                                      :element-type 'file-stream
                                      :fill-pointer 0))
(defparameter *input-files* nil)

(defparameter sp  0)
(defparameter rsp 0)
(defparameter ip  0)

(declaim (type cell sp rsp ip)
         (type (simple-array cell)
               *image*
               *addresses*
               *data*
               *ports*)
         (type (vector file-stream) *files*))

(defmacro tos () `(aref *data* sp))
(defmacro nos () `(aref *data* (max 0 (1- sp))))
(defmacro tors () `(aref *addresses* rsp))
(defmacro op (o) `(aref *image* ,o))
(defmacro drop (&optional (n 1))
  (let ((nn (gensym)))
    `(let ((,nn ,n))
       (declare (type cell ,nn))
       (if (> sp (- 1 ,nn)) (decf sp ,nn) (setf ip *image-size*)))))

#+ecl
(progn
  (defcstruct termios
              (iflag :unsigned-int)
              (oflag :unsigned-int)
              (cflag :unsigned-int)
              (lflag :unsigned-int)
              (cc :unsigned-char :count 20)
              (ispeed :int)
              (ospeed :int))
  (defcfun (%tcgetattr "tcgetattr") :int (fd :int) (term :pointer))
  (defcfun (%tcsetattr "tcsetattr") :int (fd :int) (action :int) (term :pointer)))

#+sbcl
(progn
  (define-alien-type nil
    (struct termios
            (iflag unsigned-int)
            (oflag unsigned-int)
            (cflag unsigned-int)
            (lflag unsigned-int)
            (cc (array unsigned-char 20))
            (ispeed int)
            (ospeed int)))
  (define-alien-routine "tcgetattr" int (fd int) (term (* (struct termios))))
  (define-alien-routine "tcsetattr" int (fd int) (action int) (term (* (struct termios)))))

(defvar console-parameters nil)

(defun set-console-parameters (a-icanon a-echo a-isig a-iflag)
  #-(or sbcl ecl)
  (declare (ignore a-icanon a-echo a-isig a-iflag))
  #+ecl
  (with-foreign-object (term 'termios)
    (%tcgetattr 0 term)
    (with-foreign-slots ((iflag lflag) term termios)
      (prog1 (list (ldb (byte 1 8) lflag)
                   (ldb (byte 1 3) lflag)
                   (ldb (byte 1 7) lflag)
                   iflag)
        (setf (ldb (byte 1 8) lflag) a-icanon)
        (setf (ldb (byte 1 3) lflag) a-echo)
        (setf (ldb (byte 1 7) lflag) a-isig)
        (setf iflag a-iflag)
        (%tcsetattr 0 0 term))))
  #+sbcl
  (with-alien ((term (struct termios)))
    (tcgetattr 0 (addr term))
    (let ((lflag (slot term 'lflag)))
      (prog1 (list (ldb (byte 1 8) lflag)
                   (ldb (byte 1 3) lflag)
                   (ldb (byte 1 7) lflag)
                   (slot term 'iflag))
        (setf (ldb (byte 1 8) lflag) a-icanon)
        (setf (ldb (byte 1 3) lflag) a-echo)
        (setf (ldb (byte 1 7) lflag) a-isig)
        (setf (slot term 'lflag) lflag)
        (setf (slot term 'iflag) a-iflag)
        (tcsetattr 0 0 (addr term))))))

(defun console-prepare ()
  #+(or sbcl ecl)
  (setf console-parameters (set-console-parameters 0 0 0 0)))

(defun console-finish ()
  (when console-parameters
    (apply #'set-console-parameters console-parameters)))

(defun console-getc ()
  (let ((c (read-char (or (car *input-files*) *standard-input*) nil)))
    (if c (char-code c)
      (prog1 0
        (if *input-files* (close (pop *input-files*))
          (setf ip *image-size*))))))

(defun console-putc (c)
  (declare (type cell c))
  (write-char (code-char c) *standard-output*)
  (force-output *standard-output*))

(defun get-string (pos)
  (with-output-to-string (fd)
    (loop
      (let ((n 0))
        (setf n (aref *image* pos))
        (when (= n 0) (return))
        (write-char (code-char n) fd)
        (incf pos)))))

(defun add-input-file (fname)
  (push (open fname :element-type 'character :direction :input)
        *input-files*))

(defun handle-devices ()
  (when (and (= (aref *ports* 0) 0) (= (aref *ports* 1) 1))
    (setf (aref *ports* 1) (console-getc)
          (aref *ports* 0) 1))
  (when (= (aref *ports* 2) 1)
    (console-putc (tos)) (drop)
    (setf (aref *ports* 2) 0 (aref *ports* 0) 1))
  (when (/= (aref *ports* 4) 0)
    (setf (aref *ports* 0) 1)
    (let ((io-op (aref *ports* 4)))
      (setf (aref *ports* 4) 0)
      (case io-op
        ((1)
         (with-open-file (fd *image-file*
                             :element-type 'cell
                             :direction :output
                             :if-exists :supersede
                             :if-does-not-exist :create)
           (loop for d = 0 then (1+ d)
                 while (< d *image-size*)
                 do (write-byte (aref *image* d) fd))))
        ((2) (ignore-errors (add-input-file (get-string (tos)))) (drop))
        ((-1)
           (ignore-errors
             (vector-push
               (case (tos)
                 ((0) (open (get-string (nos))
                            :element-type '(unsigned-byte 8)
                            :direction :input))
                 ((1) (open (get-string (nos))
                            :element-type '(unsigned-byte 8)
                            :direction :output
                            :if-exists :supersede
                            :if-does-not-exist :create))
                 ((2) (open (get-string (nos))
                            :element-type '(unsigned-byte 8)
                            :direction :output
                            :if-exists :append
                            :if-does-not-exist :create))
                 ((3) (open (get-string (nos))
                            :element-type '(unsigned-byte 8)
                            :direction :io
                            :if-exists :overwrite
                            :if-does-not-exist :error)))
               *files*)
             (setf (aref *ports* 4) (length *files*)))
         (drop 2))
        ((-2) (ignore-errors
                (setf (aref *ports* 4) (read-byte (aref *files* (1- (tos))))))
              (drop))
        ((-3) (ignore-errors
                (write-byte (nos) (aref *files* (1- (tos))))
                (setf (aref *ports* 4) 1))
              (drop 2))
        ((-4) (ignore-errors
                (close (aref *files* (1- (tos)))))
              (drop))
        ((-5) (ignore-errors
                (setf (aref *ports* 4) (file-position (aref *files* (1- (tos))))))
              (drop))
        ((-6) (ignore-errors
                (setf (aref *ports* 4) (file-position (aref *files* (1- (tos))) (nos))))
              (drop 2))
        ((-7) (ignore-errors
                (setf (aref *ports* 4) (file-length (aref *files* (1- (tos))))))
              (drop))
        ((-8) (handler-case
                (progn (delete-file (get-string (tos)))
                       (setf (aref *ports* 4) -1))
                (error () (setf (aref *ports* 4) 0)))
              (drop))
        (otherwise nil))))
  (when (/= (aref *ports* 5) 0)
    (setf (aref *ports* 0) 1)
    (let ((io-op (aref *ports* 5)))
      (setf (aref *ports* 5) 0)
      (case io-op
        ((-1) (setf (aref *ports* 5) *image-size*))
        ((-5) (setf (aref *ports* 5) sp))
        ((-6) (setf (aref *ports* 5) rsp))
        ((-8) (setf (aref *ports* 5) (- (get-universal-time) 2208988800)))
        ((-9) (setf ip *image-size*))
        ((-10) (let ((s #+ecl  (si:getenv (get-string (tos)))
                        #+sbcl (sb-ext:posix-getenv (get-string (tos)))
                        #+clisp (ext:getenv (get-string (tos)))
                        #-(or ecl sbcl clisp) nil))
                 (declare (type (or nil simple-string) s))
                 (loop for x from 0 below (length s)
                       for d = (nos) then (1+ d)
                       do (setf (aref *image* d) (char-code (aref s x)))
                       finally (setf (aref *image* (1+ d)) 0)))
               (drop 2))
        ((-11) (setf (aref *ports* 5) 80))
        ((-12) (setf (aref *ports* 5) 25))
        ((-13) (setf (aref *ports* 5) +cell-size+))
        ((-14) (setf (aref *ports* 5)
                     #+big-endian 1
                     #-big-endian 0))))))

(defun ngaro ()
  (console-prepare)
  (unwind-protect
    (prog1 (values)
      (with-open-file (fd *image-file*
                          :element-type 'cell
                          :direction :input)
        (loop for d = 0 then (1+ d)
              for c = (read-byte fd nil)
              while (and (< d *image-size*) c)
              do (setf (aref *image* d) c)))
      (loop
        (case (op ip)
          ((0) nil)
          ((1) (incf sp) (incf ip) (setf (tos) (op ip)))
          ((2) (incf sp) (setf (tos) (nos)))
          ((3) (drop))
          ((4) (rotatef (tos) (nos)))
          ((5) (incf rsp) (setf (tors) (tos)) (drop))
          ((6) (incf sp) (setf (tos) (tors)) (decf rsp))
          ((7) (decf (tos)) (incf ip)
               (if (and (/= (tos) 0) (> (tos) -1))
                 (setf ip (1- (op ip)))
                 (drop)))
          ((8) (setf ip (1- (op (1+ ip))))
               (if (< ip 0) (setf ip *image-size*))
               (if (= 0 (op (1+ ip))) (incf ip))
               (if (= 0 (op (1+ ip))) (incf ip)))
          ((9) (setf ip (tors)) (decf rsp) (if (< ip 0) (setf ip *image-size*)))
          ((10) (incf ip) (if (>  (nos) (tos)) (setf ip (1- (op ip)))) (drop 2))
          ((11) (incf ip) (if (<  (nos) (tos)) (setf ip (1- (op ip)))) (drop 2))
          ((12) (incf ip) (if (/= (nos) (tos)) (setf ip (1- (op ip)))) (drop 2))
          ((13) (incf ip) (if (=  (nos) (tos)) (setf ip (1- (op ip)))) (drop 2))
          ((14) (setf (tos) (op (tos))))
          ((15) (setf (op (tos)) (nos)) (drop 2))
          ((16) (setf (nos) (+ (nos) (tos))) (drop))
          ((17) (setf (nos) (- (nos) (tos))) (drop))
          ((18) (setf (nos) (* (nos) (tos))) (drop))
          ((19) (if (/= 0 (tos))
                  (multiple-value-bind (a b) (truncate (nos) (tos))
                    (setf (tos) a (nos) b))
                  (setf (tos) 0 (nos) 0)))
          ((20) (setf (nos) (logand (tos) (nos))) (drop))
          ((21) (setf (nos) (logior (tos) (nos))) (drop))
          ((22) (setf (nos) (logxor (tos) (nos))) (drop))
          ((23) (setf (nos) (ash (nos) (tos))) (drop))
          ((24) (setf (nos) (ash (nos) (- (tos)))) (drop))
          ((25) (when (= (tos) 0) (drop) (setf ip (tors)) (decf rsp)))
          ((26) (incf (tos)))
          ((27) (decf (tos)))
          ((28) (setf (tos) (prog1 (aref *ports* (tos))
                              (setf (aref *ports* (tos)) 0))))
          ((29) (setf (aref *ports* 0) 0
                      (aref *ports* (tos)) (nos))
                (drop 2))
          ((30) (handle-devices))
          (otherwise (incf rsp)
                     (setf (tors) ip ip (1- (op ip)))
                     (if (< ip 0) (setf ip *image-size*))
                     (if (= 0 (op (1+ ip))) (incf ip))
                     (if (= 0 (op (1+ ip))) (incf ip))))
        (incf ip) (if (>= ip *image-size*) (return nil))))
    (console-finish)))

; vim:lw+=with-foreign-object,with-foreign-slots,with-alien,define-alien-type:
