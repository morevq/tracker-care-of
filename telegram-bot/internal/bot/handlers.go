package bot

import (
	tele "gopkg.in/telebot.v3"
)

func registerHandlers(b *tele.Bot) {
	b.Handle("/start", onStart)
	b.Handle("/help", onHelp)
	
	b.Handle(tele.OnText, onUnknownText)
}

func onStart(c tele.Context) error {
	return c.Send("Привет! Я бот.\nНапиши /help чтобы увидеть список команд.")
}

func onHelp(c tele.Context) error {
	msg := "Доступные команды:\n" +
		"/start — начать\n" +
		"/help — помощь\n"
	return c.Send(msg)
}

func onUnknownText(c tele.Context) error {
	return c.Send("Не понял команду. Попробуй /help")
}
