#include<stdexcept> //Provides standard exceptions such as std::runtime_error.

#include "atlas/market_data/binary_recorder.hpp"
#include "atlas/market_data/recording.hpp"


// It takes market events (Trades, Quotes, and Order Book Updates) and saves them into a binary file.

namespace atlas::market_data{
    // A destructor is a special function that runs automatically when an object is destroyed.
    BinaryMarketDataRecorder::~BinaryMarketDataRecorder(){
        close();
    }

    // This function opens a file where market data will be stored.
    void BinaryMarketDataRecorder::open(const std::filesystem::path &path) {
        close();  //If another file was already open, close it first.

        output_.open(
            path,
            std::ios::binary | std::ios::trunc  //std::ios::trunc means erase the existing contents if the file already exists.
        );

        if(!output_.is_open()){
            throw std::runtime_error(
                "failed to open market_data recording"
            );
        }

        write_u32(kRecordingMagic);  // check weather file is valid markeet-data recording
        write_u16(kRecordingVersion);  //Tells the reader which version of the recording format was used.

    }

    void BinaryMarketDataRecorder::record(const MarketEvent &event){
        if(!output_.is_open()){
            throw std::runtime_error(
                "recorder is not open"
            );
        }

        std::visit(
            [this](const auto &value){
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T , Trade>) {
                    write_trade(value);
                }else if constexpr (std::is_same_v<T , Quote>){
                    write_quote(value);
                }else if constexpr (std::is_same_v<T,OrderBookUpdate>){
                    write_order_book_update(value);
                }
            },
            event
        );

        if (!output_){
            throw std::runtime_error(
                "failed while writing market-data recording"
            );
        }
    }

    void BinaryMarketDataRecorder::close(){
        if (output_.is_open()) {
            output_.flush();
            output_.close();
        }
    }

    void BinaryMarketDataRecorder::write_u8(std::uint8_t value){
        output_.put(static_cast<char>(value));
    }

    void BinaryMarketDataRecorder::write_u32(std::uint32_t value){
        for(std::size_t i = 0; i < sizeof(value); i++){
            write_u8(
                static_cast<std::uint8_t>(
                    (value >> (8*i) & 0xFFU)
                )
            );
        }
    }

    void BinaryMarketDataRecorder::write_u16(
    std::uint16_t value
) {
    for (std::size_t i = 0; i < sizeof(value); ++i) {
        write_u8(
            static_cast<std::uint8_t>(
                (value >> (8 * i)) & 0xFFU
            )
        );
    }
}

    void BinaryMarketDataRecorder::write_i64(std::int64_t value){
        write_u64(
            static_cast<std::uint64_t>(value)
        );
    }

    void BinaryMarketDataRecorder::write_u64(std::uint64_t value){
        for(std::size_t i = 0 ; i < sizeof(value); i++){
            write_u8(
                static_cast<std::uint8_t>(
                    (value >> (8 * i)) & 0xFFULL
                )
            );
        }
    }

    void BinaryMarketDataRecorder::write_trade(const Trade &trade){
        constexpr std::uint32_t record_length = 29;

        write_u32(record_length);
        write_u8(
            static_cast<std::uint8_t>(
                RecordedEventType::Trade
            )
        );

        write_u32(trade.instrument_id);
        write_i64(trade.price);
        write_u64(trade.quantity);
        write_u64(trade.timestamp);
    }

    void BinaryMarketDataRecorder::write_quote(
    const Quote& quote
) {
    constexpr std::uint32_t record_length = 45;

    write_u32(record_length);
    write_u8(
        static_cast<std::uint8_t>(
            RecordedEventType::Quote
        )
    );

    write_u32(quote.instrument_id);
    write_i64(quote.bid_price);
    write_u64(quote.bid_quantity);
    write_i64(quote.ask_price);
    write_u64(quote.ask_quantity);
    write_u64(quote.timestamp);
}

void BinaryMarketDataRecorder::write_order_book_update(
    const OrderBookUpdate& update
) {
    constexpr std::uint32_t record_length = 26;

    write_u32(record_length);
    write_u8(
        static_cast<std::uint8_t>(
            RecordedEventType::OrderBookUpdate
        )
    );

    write_u32(update.instrument_id);

    write_u8(
        static_cast<std::uint8_t>(
            update.side
        )
    );

    write_i64(update.price);
    write_u64(update.quantity);
    write_u64(update.timestamp);
}

}
