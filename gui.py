from tkinter import *
import subprocess
import sys

class Window(Frame):
    def __init__(self,master):
        Frame.__init__(self, master)
        self.master = master
        # self.client_ip = client_ip
        self.init_window()

    def init_window(self):
        self.master.title("Macro Manager")
        self.pack(fill=BOTH)
        heading = Label(self, text="Keyboard Macro Manager", font="Verdana 20 bold", fg="green")
        heading.pack()

        options = Label(self, text="Select an option from below: ", font="Verdana 12", fg="blue")
        options.pack(fill=X, pady=20)
        
        add_macro_button = Button(self, text="Add New Macro", command=lambda: self.add_macro_window(), bg="gray")
        add_macro_button.pack(pady=20, fill=X, padx=100)

        show_macro_button = Button(self, text="Show Macros", bg="gray")
        show_macro_button.pack(pady=20, fill=X, padx=100)

        fetch_macro_button = Button(self, text="Fetch Macros from Server", command= lambda: self.fetch_macro_window(), bg="gray")
        fetch_macro_button.pack(pady=20, fill=X, padx=100)

        push_macro_button = Button(self, text="Push Macros to server", command= lambda: self.push_macro_window(), bg="gray")
        push_macro_button.pack(pady=20, fill=X, padx=100)


    def add_macro_window(self):
        newWindow = Toplevel(self.master)
        label = Label(newWindow, text='Add New Macro', font="Verdana 15", fg="blue")
        label.pack()

        startButton = Button(newWindow, text="Start Combination", command=lambda: self.write_to_proc_file('start'), bg="gray")
        startButton.pack(pady=10)

        endButton = Button(newWindow, text="End Combination", command=lambda: self.write_to_proc_file('end'), bg="gray")
        endButton.pack(pady=10)

        startMacroButton = Button(newWindow, text="Start Macro", command=lambda: self.write_to_proc_file('startFullMacro'), bg="gray")
        startMacroButton.pack(pady=10)

        endMacroButton = Button(newWindow, text="End Macro", command=lambda: self.write_to_proc_file('endFullMacro'), bg="gray")
        endMacroButton.pack(pady=10)

        backButton = Button(newWindow, text="Back", command=newWindow.destroy, bg="red")
        backButton.pack(pady=50)
        newWindow.geometry("300x300")
        

    def write_to_proc_file(self, cmd):
        subprocess.check_call(["./write_proc", cmd])


    def fetch_macro_window(self):
        subprocess.check_call(["sudo", "rmmod", "kmodule"])
        # subprocess.check_call(["./client", self.client_ip, "PULL"])
        subprocess.check_call(["sudo", "insmod", "kmodule.ko"])

        newWindow = Toplevel(self.master)
        label = Label(newWindow, text='Fetched.', font="Verdana 15", fg="blue")
        label.pack()
        newWindow.geometry("200x200")
        backButton = Button(newWindow, text="Back", command=newWindow.destroy, bg="red")
        backButton.pack(pady=50)
    

    def push_macro_window(self):
        # subprocess.check_call(["./client", self.client_ip, "PUSH"])

        newWindow = Toplevel(self.master)
        label = Label(newWindow, text='Pushed to Server.', font="Verdana 15", fg="blue")
        label.pack()
        newWindow.geometry("200x200")
        backButton = Button(newWindow, text="Back", command=newWindow.destroy, bg="red")
        backButton.pack(pady=50)

        

if __name__ == '__main__':
    root = Tk()
    root.geometry("400x400")
    app = Window(root)
    root.mainloop()