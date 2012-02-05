;; Ngaro VM in Scheme (R5RS)
;; Copyright (c) 2011, Christian Kellermann
;; Build using Chicken Scheme (4.7.0):
;;
;;   csc retro.scm
;;
;; Non-standard features:
;;  read-byte, case, iota, fprintf,
;;  get-environment-variable and probably others

(use numbers extras srfi-1)

;; machine declarations
(define *m* (make-vector 1000000 0))
(define *data* (make-vector 1024 0))
(define *addr* (make-vector 128 0))
(define *ip* 0)
(define *dataptr* 0)
(define *addrptr* 0)

;; convenience procedures
(define (dataptr+)
  (set! *dataptr* (add1 *dataptr*)))

(define (dataptr-)
  (set! *dataptr* (sub1 *dataptr*)))

(define (tos) (vector-ref *data* (sub1 *dataptr*)))

(define (ip #!optional (addr *ip*))
  (vector-ref *m* addr))

(define (ip+)
  (if (> (add1 *ip*)
         1000000)
      (exit)
      (set! *ip* (add1 *ip*))))

(define (ip-)
  (set! *ip* (sub1 *ip*)))

(define (pushd x)
  (vector-set! *data* *dataptr* (normalize x))
  (dataptr+))

(define (popd)
  (dataptr-)
  (vector-ref *data* *dataptr*))

(define (toas)
  (vector-ref *addr* (sub1 *addrptr*)))

(define (addrptr+)
  (set! *addrptr* (add1 *addrptr*)))

(define (addrptr-)
  (set! *addrptr* (sub1 *addrptr*)))

(define (pusha x)
  (vector-set! *addr* *addrptr* (normalize x))
  (addrptr+))

(define (popa)
    (addrptr-)
    (vector-ref *addr* *addrptr*))

(define *ports* (make-vector 12 0))

(define (bytes->cell b1 b2 b3 b4)
  (normalize (+ b1
                (* b2 256)
                (* b3 256 256)
                (* b4 256 256 256))))

(define (load-image filename)
  (with-input-from-file
      filename
    (lambda ()
      (let loop ()
        (let* ((b1 (read-byte))
               (b2 (read-byte))
               (b3 (read-byte))
               (b4 (read-byte)))
          (if (not (eof-object? b4))
              (begin
                (vector-set! *m* *ip* (bytes->cell b1 b2 b3 b4))
                (ip+)
                (loop))))))))

(define (normalize val)
  (if (< 2147483647 val)
      (inexact->exact (- val 4294967296))
      val))

(define (string-from-memory ptr)
  (let loop ((i ptr)
             (res '()))
    (let ((c (vector-ref *m* ptr)))
      (case c
       ((#\0) (list->string (reverse res)))
       (else (loop (add1 i) (cons c res)))))))

(define (push-string str addr)
  (for-each (lambda (c a)
              (vector-set! *m* a (char->integer)))
            (string->list str)
            (iota (string-length str) addr))
  (vector-set! (+ addr (string-length str))
                  0))

(define (display-cell c)
  (if (> c 0)
      (display (integer->char c))
      (display "\x1b[2J\x1b[1;1H"))
  (flush-output (current-output-port))
  (when (= c 8)
      (display (integer->char 32))
      (flush-output (current-output-port))
      (display (integer->char 8)))
  (flush-output (current-output-port)))

(define (read-to-cell)
  (let ((c (read-char)))
    (if (eof-object? c)
        (exit 0)
        (begin
          (flush-output)
          (char->integer c)))))

;;I/O ports


(define (process-port p)
  (let ((op (vector-ref *ports* p)))
    (case p
      ((0) 0)
      ((1) (read-to-cell))
      ((2) (display-cell (popd)) 0)
      ((3) (flush-output) 0) ;; refresh video XXX implement me please!
      ((4) 0) ;; file i/o
      ((5) ; query VM
       (case op
         ((-1) 1000000) ; memsize
         ((-2) 0) ; canvas exist?
         ((-3) 0) ; canvas width
         ((-4) 0) ; canvas height
         ((-5) *dataptr*) ; data stack depth
         ((-6) *addrptr*) ; address stack depth
         ((-7) 0) ; mouse?
         ((-8) (current-seconds))
         ((-9) (set! *ip* (sub1 1000000)))
         ((-10)(list (or
                       (get-environment-variable (string-from-memory (popd)))
                       "")
                      (popd)))
         ((-11) 0) ; terminal width
         ((-12) 0) ; terminal height
         (else 0)))
      ((6) 0) ;; XXX canvas
      ((7) 0) ;; XXX mouse
      (else 0))))

(define (process-ports)
  (if (= 0 (vector-ref *ports* 0))
      (begin
        (do ((i 0 (add1 i)))
            ((> i 7) #t)
          (if (not (equal? 0 (vector-ref *ports* i)))
              (begin
                (vector-set! *ports* i (process-port i))
                (vector-set! *ports* 0 1)))))
      (fprintf (current-error-port) "Nothing to do!~%")))

(define (read-from-port p)
  (let ((res (vector-ref *ports* p)))
    (cond ((pair? res)
           (for-each pushd res))
          ((string? res)
           (push-string res))
          (else
           (pushd res))))
  (vector-set! *ports* p 0))

;; Instructions
(define (process-instruction)
  (case (ip)
    ((0)
     #t) ; NOP
    ((1)
     (ip+)
     (pushd (ip))) ; LIT
    ((2)
     (pushd (tos))) ; DUP
    ((3)
     (popd)) ; DROP
    ((4)
     (let* ((d1 (popd))
            (d2 (popd)))
       (pushd d1)
       (pushd d2))) ; SWAP
    ((5)
     (pusha (popd))) ; PUSH
    ((6)
     (pushd (popa))) ; POP
    ((7)
     (pushd (sub1 (popd)))
     (ip+)
     (if (< 0 (tos))
         (set! *ip* (sub1 (ip)))
         (popd))) ; LOOP
    ((8)
     (ip+)
     (set! *ip* (sub1 (ip)))) ; JUMP
    ((9)
     (set! *ip* (popa))) ; RETURN
    ((10)
     (ip+)
     (let* ((a (popd))
            (b (popd)))
       (if (< a b)
           (set! *ip* (sub1 (ip)))))) ; LT_JUMP
    ((11)
     (ip+)
     (let* ((a (popd))
            (b (popd)))
       (if (> a b)
           (set! *ip* (sub1 (ip)))))) ; GT_JUMP
    ((12)
     (ip+)
     (let* ((a (popd))
            (b (popd)))
       (if (not (= a b))
           (set! *ip* (sub1 (ip)))))) ; NE_JUMP
    ((13)
     (ip+)
     (let* ((a (popd))
            (b (popd)))
       (if (= a b)
           (set! *ip* (sub1 (ip)))))) ; EQ_JUMP
    ((14)
     (pushd (ip (popd)))) ; FETCH
    ((15)
     (let* ((addr (popd))
            (val (popd)))
       (vector-set! *m* addr val))) ; STORE
    ((16)
     (pushd (+ (popd) (popd)))) ; ADD
    ((17)
     (let* ((d1 (popd))
            (d2 (popd)))
       (pushd (- d2 d1)))) ; SUB
    ((18)
     (pushd (* (popd) (popd)))) ; MUL
    ((19)
     (let* ((b (popd))
            (a (popd)))
       (pushd (remainder a b))
       (pushd (quotient a b)))) ; DIVMOD
    ((20)
     (pushd (bitwise-and (popd) (popd)))) ; AND
    ((21)
     (pushd (bitwise-ior (popd) (popd)))) ; OR
    ((22)
     (pushd (bitwise-xor (popd) (popd)))) ; XOR
    ((23)
     (let* ((b (popd))
            (a (popd)))
       (pushd (arithmetic-shift a b)))) ; SHL
    ((24)
     (let* ((b (popd))
            (a (popd)))
       (pushd (arithmetic-shift a (- b))))) ; SHR
    ((25)
     (if (zero? (tos))
         (begin
           (popd)
           (set! *ip* (popa))))) ; ZERO_EXIT
    ((26)
     (pushd (add1 (popd)))) ; INC
    ((27)
     (pushd (sub1 (popd)))) ; DEC
    ((28)
     (read-from-port (popd))) ; IN
    ((29)
     (let* ((p (popd))
            (val (popd)))
     (vector-set! *ports* p val))) ; OUT
    ((30)
     (process-ports)) ; WAIT
    (else
     (pusha *ip*) ; implicit call
     (set! *ip*
           (do ((i (ip) (add1 i)))
               ((not (equal? (ip i) 0)) (sub1 i)))))))


(define (run)
  (set! *ip* 0)
  (let loop ()
    (when (< *ip* 1000000)
      (process-instruction)
      (ip+)
      (loop))))

(load-image "retroImage")
(run)
