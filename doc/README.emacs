;;
;; sample .emacs file
;;

(setq auto-mode-alist (cons '("\\.spec$" . c++-mode) auto-mode-alist))
(setq auto-mode-alist (cons '("\\.qc$" . c++-mode) auto-mode-alist))
(setq auto-mode-alist (cons '("\\.c$" . c++-mode) auto-mode-alist))
(setq auto-mode-alist (cons '("\\.h$" . c++-mode) auto-mode-alist))

;; don't make the *~ backups
(setq-default make-backup-files nil)

;; don't remember what this does
(setq-default transient-mark-mode t)

(global-set-key (quote [67108899]) (quote goto-line))
(global-set-key (quote [67108922]) (quote goto-line))
(global-set-key (quote [67108907]) (quote font-lock-fontify-buffer))
(cond
 (window-system
  (add-hook 'c-mode-hook 'font-lock-mode)
  (add-hook 'makefile-mode-hook 'font-lock-mode)
  (add-hook 'tex-mode-hook 'font-lock-mode)
  (add-hook 'html-mode-hook 'font-lock-mode)
  (add-hook 'lisp-mode-hook 'font-lock-mode)
  (add-hook 'c++-mode-hook 'font-lock-mode)
  (add-hook 'emacs-lisp-mode-hook 'font-lock-mode)
  (add-hook 'fortran-mode-hook 'font-lock-mode)
  (add-hook 'perl-mode-hook 'font-lock-mode)
  )
 )
(add-hook 'c-mode-hook 'c-toggle-auto-state)
(add-hook 'c++-mode-hook '(lambda()
                            (c-set-style "stroustrup")))          

(put 'upcase-region 'disabled nil)
