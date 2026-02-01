package bot

import (
	"time"

	tele "gopkg.in/telebot.v3"
)

func New(token string) (*tele.Bot, error) {
	pref := tele.Settings{
		Token:  token,
		Poller: &tele.LongPoller{Timeout: 10 * time.Second},
	}

	b, err := tele.NewBot(pref)
	if err != nil {
		return nil, err
	}
	
	b.Use(recoverMiddleware())
	b.Use(loggingMiddleware())
	b.Use(errorResponderMiddleware())

	registerHandlers(b)

	return b, nil
}
