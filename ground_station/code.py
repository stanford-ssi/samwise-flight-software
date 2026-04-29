from radio_initialization import initialize
from ui import debug_listen_mode, interactive_command_loop


def main():
    """Main program entry point"""
    print("=== Samwise Ground Station ===")
    print("Interactive LoRA Communication System (Refactored)")

    # Initialize radio
    initialize()

    print("\nSelect mode:")
    print("1. Merry (UHF): 433 Debug Listen Mode (watch for any packets)")
    print("2. Merry (UHF): 433 Interactive Command Mode")
    print("3. Pippin (S-Band): 2400 Listen Mode")

    try:
        choice = input("Enter choice (1 or 2): ").strip()
        if choice == "1":
            print("\n=== Starting Merry Board | 433 Debug Listen Mode ===")
            debug_listen_mode()
        elif choice == "2":
            print("\n=== Starting Merry Board | 433 Interactive Command Loop ===")
            interactive_command_loop()
        elif choice == "3":
            print("\n=== Starting Pippin Board | 2400 Listen Mode")
    except KeyboardInterrupt:
        print("\nGoodbye!")


if __name__ == "__main__":
    try:
        import functools

        # Override print to flush immediately (helpful for some serial monitors)
        print = functools.partial(print, flush=True)
    except ImportError:
        pass
    except Exception as e:
        print(f"Error setting up print flushing: {e}")

    main()
