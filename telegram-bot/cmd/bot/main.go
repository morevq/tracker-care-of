package main

import (
	"context"
	"log"
	"os"
	"os/signal"
	"syscall"

	"telegram-bot/internal/bot"
)

func main() {
	token := os.Getenv("BOT_TOKEN")
	if token == "" {
		log.Fatal("BOT_TOKEN env is required")
	}

	b, err := bot.New(token)
	if err != nil {
		log.Fatal(err)
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	go func() {
		<-ctx.Done()
		log.Println("shutting down bot...")
		b.Stop()
	}()

	log.Println("bot is running")
	b.Start()
}
