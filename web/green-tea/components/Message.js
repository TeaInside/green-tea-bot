function Message() {
    return (
        <div className="flex">
            <img
                className="w-12 h-12 rounded-full"
                src="profile.jpeg"
                alt="profile"
            />
            <div className="ml-4 space-y-2">
                <p className="bg-gray-200  px-4 py-2 max-w-md rounded-xl whitespace-pre-line">
                    Bro, kabar e piye?
                </p>
                <p className="bg-gray-200 px-4 py-2 max-w-md rounded-xl whitespace-pre-line">
                    Aku neng kene ngeleh Lorem ipsum dolor sit amet consectetur
                    adipisicing elit. Nemo optio mollitia aspernatur adipisci
                    nostrum molestias eos illum nesciunt neque at natus in,
                    distinctio aliquam sit libero harum, quaerat aliquid
                    deserunt.
                </p>
                <p className="bg-gray-200 px-4 py-2 max-w-md rounded-xl whitespace-pre-line">
                    Njaluk tulung tukok ne nasi padang
                </p>
            </div>
        </div>
    );
}

export default Message;
