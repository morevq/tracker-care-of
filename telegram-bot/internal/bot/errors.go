package bot

import (
	"errors"
	"log"

	tele "gopkg.in/telebot.v3"
)

func handleError(where string, err error, c tele.Context) {
	log.Printf("[%s] err=%v", where, err)

	if c == nil {
		return
	}
	_ = safeSend(c, "Произошла ошибка при обработке запроса. Попробуй снова.")
}

func safeSend(c tele.Context, msg string) error {
	if c == nil {
		return errors.New("nil context")
	}
	_, err := c.Bot().Send(c.Chat(), msg)
	return err
}
