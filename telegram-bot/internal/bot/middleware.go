package bot

import (
	"fmt"
	"log"
	"runtime/debug"
	"time"

	tele "gopkg.in/telebot.v3"
)

func loggingMiddleware() tele.MiddlewareFunc {
	return func(next tele.HandlerFunc) tele.HandlerFunc {
		return func(c tele.Context) error {
			start := time.Now()
			err := next(c)

			u := c.Sender()
			user := "unknown"
			if u != nil {
				user = fmt.Sprintf("%s (%d)", u.Username, u.ID)
			}

			text := ""
			if m := c.Message(); m != nil {
				text = m.Text
			}

			if err != nil {
				log.Printf("[ERR] user=%s text=%q dur=%s err=%v", user, text, time.Since(start), err)
			} else {
				log.Printf("[OK ] user=%s text=%q dur=%s", user, text, time.Since(start))
			}
			return err
		}
	}
}

func recoverMiddleware() tele.MiddlewareFunc {
	return func(next tele.HandlerFunc) tele.HandlerFunc {
		return func(c tele.Context) (err error) {
			defer func() {
				if r := recover(); r != nil {
					log.Printf("[PANIC] %v\n%s", r, string(debug.Stack()))
					_ = safeSend(c, "Упс, внутренняя ошибка. Попробуй ещё раз чуть позже.")
					err = fmt.Errorf("panic: %v", r)
				}
			}()
			return next(c)
		}
	}
}

func errorResponderMiddleware() tele.MiddlewareFunc {
	return func(next tele.HandlerFunc) tele.HandlerFunc {
		return func(c tele.Context) error {
			err := next(c)
			if err != nil {
				_ = safeSend(c, "Произошла ошибка при обработке запроса. Попробуй снова.")
			}
			return err
		}
	}
}
