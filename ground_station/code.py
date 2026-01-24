import sys
import gs

def main():
    """Main program entry point"""
    print("=== Samwise Ground Station ===")
    print("Interactive LoRA Communication System (Refactored)")
    
    # Initialize radio
    gs.initialize()
    
    print("\nSelect mode:")
    print("1. Debug Listen Mode (watch for any packets)")
    print("2. Interactive Command Mode")
    
    try:
        choice = input("Enter choice (1 or 2): ").strip()
        if choice == "1":
            gs.debug_listen_mode()
        else:
            print("\n=== Starting Interactive Command Loop ===")
            gs.interactive_command_loop()
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
