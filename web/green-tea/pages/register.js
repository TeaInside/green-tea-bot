import Head from "next/head";

function register() {
    return (
        <div className="w-screen bg-cream h-screen pt-8">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>

            <main className="flex flex-col w-10/12 md:w-7/12 lg:w-6/12 xl:w-5/12 2xl:w-4/12 bg-white mx-auto px-12 py-12  space-y-5">
                <img
                    className="w-[300px] mx-auto"
                    src="greentea.svg"
                    alt="logo"
                />
                <h1 className="text-gray-600 font-sans font-semibold text-[30px] text-center">
                    Create your account
                </h1>
                <form className="flex flex-col space-y-5" action="" method="">
                    <div className="flex flex-col">
                        <label
                            className="text-gray-500 text-[20px] font-sans"
                            htmlFor="email"
                        >
                            Email
                        </label>
                        <input
                            className="outline-none border-2 rounded-md px-3 py-3 mt-2 focus:border-dark-green"
                            type="text"
                            id="email"
                            placeholder="Your email"
                            required
                        />
                    </div>
                    <div className="flex flex-col">
                        <label
                            className="text-gray-500 text-[20px] font-sans"
                            htmlFor="password"
                        >
                            Username
                        </label>
                        <input
                            className="outline-none border-2 rounded-md px-3 py-3 mt-2 focus:border-dark-green"
                            type="text"
                            id="password"
                            placeholder="Your username"
                            required
                        />
                    </div>
                    <div className="flex flex-col">
                        <label
                            className="text-gray-500 text-[20px] font-sans"
                            htmlFor="password"
                        >
                            Password
                        </label>
                        <input
                            className="outline-none border-2 rounded-md px-3 py-3 mt-2 focus:border-dark-green"
                            type="password"
                            id="password"
                            placeholder="Your password"
                            required
                        />
                    </div>
                    <div className="flex flex-col">
                        <label
                            className="text-gray-500 text-[20px] font-sans"
                            htmlFor="password"
                        >
                            Retype Password
                        </label>
                        <input
                            className="outline-none border-2 rounded-md px-3 py-3 mt-2 focus:border-dark-green"
                            type="password"
                            id="password"
                            placeholder="Your password again"
                            required
                        />
                    </div>
                    <div className="flex flex-col">
                        <label
                            className="text-gray-500 text-[20px] font-sans"
                            htmlFor="password"
                        >
                            ID Telegram
                        </label>
                        <input
                            className="outline-none border-2 rounded-md px-3 py-3 mt-2 focus:border-dark-green"
                            type="text"
                            id="password"
                            placeholder="Your ID Telegram"
                            required
                        />
                    </div>
                    <a
                        className="bg-dark-green text-white text-[20px] px-3 py-4 rounded-lg text-center cursor-pointer hover:bg-dark-green-1"
                        href="/"
                    >
                        Create
                    </a>
                </form>
            </main>
        </div>
    );
}

export default register;
